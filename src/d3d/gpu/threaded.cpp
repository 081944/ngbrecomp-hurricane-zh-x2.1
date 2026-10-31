// The renderer behind a recording of the title's calls (docs/renderer.md). The title's thread records each call with
// what the console takes at call time -- the state that changed, the resource headers it names, its indices and
// rects, the game's side of it (gpu::Peek) -- and the recording is drawn from: vertex, texture and surface memory are
// read when the draw runs, as the console's GPU reads them after the call returns. Adapted from TGP482's Xbox XBE
// Recomp SDK (runtime/gpu/render_thread.cpp; see NOTICE): the resource headers are kept as copies the renderer reads
// through (gpu/headers.h) instead of copies at other addresses, the game's side and the quick menu travel with the
// calls, and the GPU emulation is what the title waits on: its push buffer is read only as far as the recording has
// been drawn (nv2a_pfifo_gate), so every reference, fence and semaphore the title waits on follows the drawing.
//
// NGB_D3D_THREAD=1: drawn on a thread of its own, the title recording one frame while the one before it draws.
// NGB_D3D_THREAD=capture: each call drawn from its recording at once, on the title's thread (a test of the recording).
#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

#include "gpu/backend.h"
#include "gpu/headers.h"
#include "kernel/kernel.h"

extern "C" {
void nv2a_pfifo_gate(int on, uint32_t put);   // xboxrecomp xemu/pfifo.c
uint32_t nv2a_pfifo_put(void);
extern void (*nv2a_user_read_hook)(void);     // xboxrecomp xemu/user.c
}

// NGB_D3D_GATE=0 (a test): the GPU emulation not held back
static void Gate(int on, uint32_t put) {
    static const bool off = std::getenv("NGB_D3D_GATE") && *std::getenv("NGB_D3D_GATE") == '0';
    nv2a_pfifo_gate(off ? 0 : on, put);
}

namespace gpu {

namespace {

// commands handed over at once, so the render thread keeps close behind
constexpr uint32_t kBatchCommands = 64;
// the render thread, waiting with nothing queued, takes what the title has recorded so far after this long: the
// title may be waiting on memory the GPU emulation writes (a semaphore) behind calls it has not handed over
constexpr auto kIdleTake = std::chrono::microseconds(1500);

enum class Op : uint8_t { State, Headers, Peek, Draw, Clear, Copy, Present, Flush, Gamma, VisBegin, VisEnd };

struct Cmd {
    Op op;
    uint32_t a = 0, b = 0, c = 0, d = 0;
    float f = 0;
    uint32_t data = 0, size = 0;   // bytes in Batch::bytes
    int32_t vs = -1, ps = -1;      // index in Batch::vs / ps: the shader changed
    int32_t quick = -1;            // index in Batch::quick: the frame's quick menu
    Surface s1, s2;
};

// a header copy as sent: data, when the call's own (a DrawVerticesUP's vertices: the title may reuse its memory once
// the call returns), at that offset in the batch's bytes
struct HeaderSend {
    uint32_t addr;
    uint32_t w[5];
    int32_t data = -1;
};

struct Batch {
    uint64_t id = 0;
    uint32_t put = 0;   // the title's DMA_PUT as it was handed over: the push buffer may be read this far once drawn
    std::vector<Cmd> cmds;
    std::vector<uint8_t> bytes;
    std::vector<std::shared_ptr<VertexShader>> vs;
    std::vector<std::shared_ptr<PixelShader>> ps;
    std::vector<QuickPeek> quick;
};

// The plain data in State around its shader pointers, sent as it changes in 64 byte pieces. Of the simple render
// states only those the renderer reads (gpu/state.h's kM...: the pixel shader's combiners, alpha test to depth offset,
// the combiner constants, depth clip control), of the transforms only those drawn with: view and projection (2, 3),
// the textures' (16-19) and the world matrices (256-259).
struct Range {
    size_t off, len;
};
size_t At(const State& s, const void* p) {
    return size_t(static_cast<const uint8_t*>(p) - reinterpret_cast<const uint8_t*>(&s));
}
std::vector<Range> StateRanges() {
    static const State s;
    auto simple = [&](uint32_t first, uint32_t last) {   // methods first to last
        return Range{At(s, &s.simple[first >> 2]), size_t(last - first) + 4};
    };
    return {
        simple(kMPsAlphaInputs, kMPsFinalEfgh),
        simple(kMAlphaTestEnable, kMDepthOffset),
        simple(kMPsConstant0, kMPsRgbInputs + 0x1C),
        simple(kMPsCompareMode, kMPsCompareMode),
        simple(kMDepthClipControl, kMDepthClipControl),
        simple(kMPsFinalConstant0, kMPsInputTexture),
        {At(s, &s.rs), At(s, &s.index_buffer) - At(s, &s.rs)},          // render states, textures, the streams
        {At(s, &s.index_buffer), At(s, &s.vs) - At(s, &s.index_buffer)},
        {At(s, &s.vsc), At(s, &s.transforms) - At(s, &s.vsc)},            // constants, viewport, surfaces, scissors
        {At(s, &s.transforms[2]), 2 * sizeof(s.transforms[0])},
        {At(s, &s.transforms[16]), 4 * sizeof(s.transforms[0])},
        {At(s, &s.transforms[256]), 4 * sizeof(s.transforms[0])},
        {At(s, &s.material), sizeof(State) - At(s, &s.material)},
    };
}

constexpr size_t kPiece = 64;

class Recorded;
Recorded* g_recorded;   // for the title's progress reads (nv2a_user_read_hook)

class Recorded final : public Backend {
public:
    Recorded(std::unique_ptr<Backend> inner, bool thread) : inner_(std::move(inner)), ranges_(StateRanges()), thread_(thread) {
        cur_ = NewBatch();
        if (thread_) {
            g_recorded = this;
            Gate(1, nv2a_pfifo_put());   // what the title put before the first recording: read
            nv2a_user_read_hook = [] { if (g_recorded) g_recorded->TitleWaits(); };
            std::thread([this] { Run(); }).detach();
        }
    }
    ~Recorded() override {
        if (!thread_) return;
        nv2a_user_read_hook = nullptr;
        g_recorded = nullptr;
        {
            std::lock_guard lock(m_);
            stop_ = true;
        }
        cv_.notify_all();
        nv2a_pfifo_gate(0, 0);
        std::unique_lock lock(m_);
        // at exit the thread may be gone mid-batch: then the renderer is left, not freed under it
        if (!done_cv_.wait_for(lock, std::chrono::milliseconds(500), [&] { return exited_; })) (void)inner_.release();
    }

    bool Init(uint32_t width, uint32_t height) override { return inner_->Init(width, height); }

    void Draw(const State& s, uint32_t prim, uint32_t start, uint32_t count, const uint8_t* indices) override {
        std::lock_guard lock(cap_);
        SendState(s, start + count);
        Cmd c{Op::Draw};
        c.a = prim;
        c.b = start;
        c.c = count;
        if (indices) {
            c.d = 1;
            c.data = Put(indices, size_t(count) * 2);
        }
        cur_->cmds.push_back(c);
        if (!thread_ || cur_->cmds.size() >= kBatchCommands) Submit();
    }

    void Clear(const State& s, uint32_t count, const uint8_t* rects, uint32_t flags, uint32_t color, float z,
               uint32_t stencil) override {
        std::lock_guard lock(cap_);
        SendState(s, 0);
        Cmd c{Op::Clear};
        c.a = count;
        c.b = flags;
        c.c = color;
        c.d = stencil;
        c.f = z;
        if (count && rects) {
            c.data = Put(rects, size_t(count) * 16);
            c.size = count * 16;
        }
        cur_->cmds.push_back(c);
        if (!thread_) Submit();
    }

    // rects and points are taken now; the copy itself runs in order
    void Copy(const Surface& src, const Surface& dst, uint32_t count, const uint8_t* src_rects,
              const uint8_t* dst_points) override {
        std::lock_guard lock(cap_);
        SendHeader(src.header);
        SendHeader(dst.header);
        FlushHeaders();
        Cmd c{Op::Copy};
        c.a = count;
        c.b = count && src_rects ? Put(src_rects, count * 16) + 1 : 0;     // (offsets + 1: 0 none)
        c.c = count && dst_points ? Put(dst_points, count * 8) + 1 : 0;
        c.s1 = src;
        c.s2 = dst;
        cur_->cmds.push_back(c);
        if (!thread_) Submit();
    }

    void Present(const Surface& front) override {
        uint64_t id;
        {
            std::lock_guard lock(cap_);
            SendHeader(front.header);
            FlushHeaders();
            SendPeek(sent_);
            Cmd c{Op::Present};
            c.s1 = front;
            c.quick = int32_t(cur_->quick.size());
            cur_->quick.emplace_back();
            inner_->CaptureQuick(cur_->quick.back());
            cur_->cmds.push_back(c);
            id = Submit();
        }
        // the title records one frame while the one before it draws
        if (thread_) {
            Wait(last_present_);
            last_present_ = id;
        }
    }

    void Sync(uint32_t data, uint32_t size) override {
        if (!thread_) inner_->Sync(data, size);   // (the renderer writes nothing back to the title's memory)
    }
    void SyncLate(uint32_t data) override {
        if (!thread_) inner_->SyncLate(data);
    }
    void Flush() override {
        std::lock_guard lock(cap_);
        cur_->cmds.push_back(Cmd{Op::Flush});
        Submit();
    }
    void SetGamma(const uint8_t* ramp) override {
        std::lock_guard lock(cap_);
        Cmd c{Op::Gamma};
        c.data = Put(ramp, 768);
        cur_->cmds.push_back(c);
        if (!thread_) Submit();
    }
    void VisibilityBegin() override {
        std::lock_guard lock(cap_);
        cur_->cmds.push_back(Cmd{Op::VisBegin});
        if (!thread_) Submit();
    }
    void VisibilityEnd(uint32_t report) override {
        std::lock_guard lock(cap_);
        Cmd c{Op::VisEnd};
        c.a = report;
        cur_->cmds.push_back(c);
        if (!thread_) Submit();
    }

    // The title reads the push buffer's progress (it may be waiting on it): what it has recorded since the last
    // batch goes now (Release). Skipped while a call is being recorded.
    void TitleWaits() {
        if (!cap_.try_lock()) return;
        Release();
        cap_.unlock();
    }

private:
    Batch* NewBatch() {
        Batch* b;
        {
            std::lock_guard lock(m_);
            if (free_.empty()) {
                b = new Batch;
            } else {
                b = free_.back();
                free_.pop_back();
            }
        }
        b->id = ++next_id_;
        return b;
    }

    uint32_t Put(const void* p, size_t n) {
        size_t at = (cur_->bytes.size() + 3) & ~size_t(3);
        cur_->bytes.resize(at + n);
        std::memcpy(cur_->bytes.data() + at, p, n);
        return uint32_t(at);
    }

    // a header as it is now, to the renderer's copies when it changed since it was last sent
    void SendHeader(uint32_t header) {
        if (!header) return;
        HeaderCopy now{};
        // vertex buffer, index buffer and palette headers are 12 bytes: reading 20 can run off the end of the heap
        uint32_t type = kernel::R32(header) & 0x70000;   // 0 vertex, 0x10000 index, 0x30000 palette
        std::memcpy(now.w, kernel::HostPtr(header), type < 0x20000 || type == 0x30000 ? 12 : 20);
        Recent& r = recent_[(header >> 4) & 63];   // the last few, in front of the map (its elements stay put)
        if (r.key != header) r = {header, &sent_headers_[header]};
        if (!std::memcmp(r.copy->w, now.w, sizeof now.w)) return;
        *r.copy = now;
        HeaderSend h{header, {}};
        std::memcpy(h.w, now.w, sizeof h.w);
        pending_headers_.push_back(h);
    }
    struct Recent {
        uint32_t key = 0;
        HeaderCopy* copy = nullptr;
    };
    Recent recent_[64];
    // a vertex buffer of the renderer's own, its data in the title's memory (DrawVerticesUP: Common bit 31): the
    // vertices copied, the copy's header pointing at them
    void SendUpHeader(uint32_t header, uint32_t bytes) {
        HeaderSend h{header, {}};
        std::memcpy(h.w, kernel::HostPtr(header), 12);
        h.data = int32_t(Put(kernel::HostPtr(h.w[1]), bytes));
        std::memset(sent_headers_[header].w, 0, sizeof h.w);   // (always sent: its vertices are new)
        recent_[(header >> 4) & 63] = {};
        pending_headers_.push_back(h);
    }
    void FlushHeaders() {
        if (pending_headers_.empty()) return;
        Cmd c{Op::Headers};
        c.data = Put(pending_headers_.data(), pending_headers_.size() * sizeof(pending_headers_[0]));
        c.size = uint32_t(pending_headers_.size());
        cur_->cmds.push_back(c);
        pending_headers_.clear();
    }

    // the game's side of the call, when it changed
    void SendPeek(const State& s) {
        Peek p;
        std::memset(&p, 0, sizeof p);
        inner_->CapturePeek(s, p);
        if (peek_sent_ && !std::memcmp(&p, &sent_peek_, sizeof p)) return;
        peek_sent_ = true;
        sent_peek_ = p;
        Cmd c{Op::Peek};
        c.data = Put(&p, sizeof p);
        cur_->cmds.push_back(c);
    }

    // a field of the state as the renderer has it, when it differs from what was sent
    void Delta(const void* field, const void* value, size_t n) {
        uint8_t* sent = reinterpret_cast<uint8_t*>(&sent_) + At(sent_, field);
        if (!std::memcmp(sent, value, n)) return;
        std::memcpy(sent, value, n);
        uint32_t head[2] = {uint32_t(At(sent_, field)), uint32_t(n)};
        size_t at = cur_->bytes.size();
        cur_->bytes.resize(at + 8 + n);
        std::memcpy(cur_->bytes.data() + at, head, 8);
        std::memcpy(cur_->bytes.data() + at + 8, value, n);
    }

    // what changed since the last command, in 64 byte pieces; the headers the call names; the game's side.
    // vertices: how many a non-indexed draw reads (a DrawVerticesUP's to copy)
    void SendState(const State& s, uint32_t vertices) {
        title_ = std::this_thread::get_id();
        const uint8_t* now = reinterpret_cast<const uint8_t*>(&s);
        for (int i = 0; i < 4; i++) {
            SendHeader(s.textures[size_t(i)]);
            SendHeader(s.palettes[size_t(i)]);
        }
        for (const auto& st : s.streams) {
            if (!st.vb) continue;
            if (kernel::R32(st.vb) & 0x80000000u)
                SendUpHeader(st.vb, vertices * st.stride);
            else
                SendHeader(st.vb);
        }
        SendHeader(s.rt.header);
        SendHeader(s.zs.header);
        FlushHeaders();
        SendPeek(s);
        Cmd c{Op::State};
        size_t first = cur_->bytes.size();
        const uint8_t* sent = reinterpret_cast<const uint8_t*>(&sent_);
        for (const Range& r : ranges_) {
            size_t off = r.off, end = r.off + r.len;
            for (; off + kPiece <= end; off += kPiece)
                if (std::memcmp(sent + off, now + off, kPiece)) Delta(sent + off, now + off, kPiece);
            if (off < end) Delta(sent + off, now + off, end - off);
        }
        if (s.vs.get() != sent_.vs.get()) {
            sent_.vs = s.vs;
            c.vs = int32_t(cur_->vs.size());
            cur_->vs.push_back(s.vs);
        }
        if (s.ps.get() != sent_.ps.get()) {
            sent_.ps = s.ps;
            c.ps = int32_t(cur_->ps.size());
            cur_->ps.push_back(s.ps);
        }
        if (cur_->bytes.size() == first && c.vs < 0 && c.ps < 0) return;
        c.data = uint32_t(first);
        c.size = uint32_t(cur_->bytes.size() - first);
        cur_->cmds.push_back(c);
    }

    // the batch handed over (cap_ held): drawn at once, or queued for the render thread with the title's DMA_PUT
    // as it is now -- every call whose push buffer commands are before it is in this batch or an earlier one (a call
    // is recorded before its Direct3D code writes them)
    uint64_t Submit() {
        uint64_t id = cur_->id;
        if (cur_->cmds.empty() && thread_) return id - 1;
        if (!thread_) {
            const auto* was = g_header_view;
            g_header_view = &headers_;
            ExecuteBatch(*cur_);
            g_header_view = was;
            Recycle(cur_);
            cur_ = NewBatch();
            return id;
        }
        cur_->put = nv2a_pfifo_put();
        submitted_ = id;
        {
            std::lock_guard lock(m_);
            queue_.push_back(cur_);
        }
        cv_.notify_one();
        cur_ = NewBatch();
        return id;
    }

    // Nothing the title waits on held back (cap_ held): what it has recorded handed over; with nothing recorded or
    // still to draw, the push buffer read as far as the title has put -- every call is recorded before its Direct3D
    // code writes its commands, so whatever is there now belongs to calls already drawn, or to none.
    void Release() {
        if (!cur_->cmds.empty()) {
            Submit();
        } else if (done_.load() >= submitted_) {
            Gate(1, nv2a_pfifo_put());
        }
    }

    // the title waits for a batch drawn (recording)
    void Wait(uint64_t id, bool hand_over = true) {
        if (!id || done_.load() >= id) return;
        if (hand_over && id >= cur_->id) {
            std::lock_guard lock(cap_);
            id = Submit();
        }
        std::unique_lock lock(m_);
        done_cv_.wait(lock, [&] { return done_.load() >= id; });
    }

    void Recycle(Batch* b) {
        b->cmds.clear();
        b->bytes.clear();
        b->vs.clear();
        b->ps.clear();
        b->quick.clear();
        std::lock_guard lock(m_);
        free_.push_back(b);
    }

    void Run() {
        g_header_view = &headers_;
        for (;;) {
            Batch* b = nullptr;
            {
                std::unique_lock lock(m_);
                if (!cv_.wait_for(lock, kIdleTake, [&] { return stop_ || !queue_.empty(); })) {
                    // nothing handed over for a while: what the title has recorded may be what it waits on
                    lock.unlock();
                    if (cap_.try_lock()) {
                        Release();
                        cap_.unlock();
                    }
                    continue;
                }
                if (queue_.empty()) break;
                b = queue_.front();
                queue_.pop_front();
            }
            ExecuteBatch(*b);
            uint64_t id = b->id;
            uint32_t put = b->put;
            Recycle(b);
            Gate(1, put);   // the GPU emulation reads as far as has been drawn
            {
                std::lock_guard lock(m_);
                done_.store(id);
            }
            done_cv_.notify_all();
        }
        std::lock_guard lock(m_);
        exited_ = true;
        done_cv_.notify_all();
    }

    void ExecuteBatch(Batch& b) {
        for (const Cmd& c : b.cmds) Execute(b, c);
        for (uint32_t a : hosted_) headers_[a].host = nullptr;   // (its bytes go with the batch)
        hosted_.clear();
    }

    void Execute(Batch& b, const Cmd& c) {
        const uint8_t* data = b.bytes.data() + c.data;
        switch (c.op) {
            case Op::State: {
                uint8_t* s = reinterpret_cast<uint8_t*>(&state_);
                for (const uint8_t* p = data; p < data + c.size;) {
                    uint32_t head[2];
                    std::memcpy(head, p, 8);
                    std::memcpy(s + head[0], p + 8, head[1]);
                    p += 8 + head[1];
                }
                if (c.vs >= 0) state_.vs = b.vs[size_t(c.vs)];
                if (c.ps >= 0) state_.ps = b.ps[size_t(c.ps)];
                break;
            }
            case Op::Headers: {
                for (uint32_t i = 0; i < c.size; i++) {
                    HeaderSend h;
                    std::memcpy(&h, data + i * sizeof h, sizeof h);
                    HeaderCopy& to = headers_[h.addr];
                    std::memcpy(to.w, h.w, sizeof to.w);
                    to.host = h.data >= 0 ? b.bytes.data() + h.data : nullptr;
                    if (to.host) hosted_.push_back(h.addr);
                }
                break;
            }
            case Op::Peek: {
                Peek p;
                std::memcpy(&p, data, sizeof p);
                inner_->UsePeek(p);
                break;
            }
            case Op::Draw: inner_->Draw(state_, c.a, c.b, c.c, c.d ? data : nullptr); break;
            case Op::Clear: inner_->Clear(state_, c.a, c.size ? data : nullptr, c.b, c.c, c.f, c.d); break;
            case Op::Copy:
                inner_->Copy(c.s1, c.s2, c.a, c.b ? b.bytes.data() + c.b - 1 : nullptr,
                             c.c ? b.bytes.data() + c.c - 1 : nullptr);
                break;
            case Op::Present:
                inner_->UseQuick(b.quick[size_t(c.quick)]);
                inner_->Present(c.s1);
                break;
            case Op::Flush: inner_->Flush(); break;
            case Op::Gamma: inner_->SetGamma(data); break;
            case Op::VisBegin: inner_->VisibilityBegin(); break;
            case Op::VisEnd: inner_->VisibilityEnd(c.a); break;
        }
    }

    std::unique_ptr<Backend> inner_;
    std::vector<Range> ranges_;
    const bool thread_;

    // the title's side (cap_: a call being recorded; the render thread takes the batch under it too)
    std::recursive_mutex cap_;
    Batch* cur_ = nullptr;
    std::thread::id title_;
    State sent_;   // the state as the renderer will have it
    std::unordered_map<uint32_t, HeaderCopy> sent_headers_;
    std::vector<HeaderSend> pending_headers_;
    Peek sent_peek_;
    bool peek_sent_ = false;
    uint64_t next_id_ = 0, last_present_ = 0;
    std::atomic<uint64_t> submitted_{0};   // the last batch handed over

    // the renderer's side
    State state_;
    std::unordered_map<uint32_t, HeaderCopy> headers_;
    std::vector<uint32_t> hosted_;   // headers whose data is in the batch being drawn: let go with it

    std::mutex m_;
    std::condition_variable cv_, done_cv_;
    std::deque<Batch*> queue_;
    std::vector<Batch*> free_;
    std::atomic<uint64_t> done_{0};
    bool stop_ = false, exited_ = false;
};

}  // namespace

std::unique_ptr<Backend> CreateThreadedBackend(std::unique_ptr<Backend> inner) {
    const char* e = std::getenv("NGB_D3D_THREAD");
    bool thread = !(e && !std::strcmp(e, "capture"));
    return std::make_unique<Recorded>(std::move(inner), thread);
}

}  // namespace gpu
