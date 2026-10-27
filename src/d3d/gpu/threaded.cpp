// The renderer behind a recording of the title's calls (docs/renderer.md). The title's thread records each call with
// what the console takes at call time -- the state that changed, the resource headers it names, its indices and
// rects, the game's side of it (gpu::Peek) -- and the recording is drawn from: vertex, texture and surface memory are
// read when the draw runs, as the console's GPU reads them after the call returns. Adapted from TGP482's Xbox XBE
// Recomp SDK (runtime/gpu/render_thread.cpp; see NOTICE): the resource headers are kept as copies the renderer reads
// through (gpu/headers.h) instead of copies at other addresses, and the game's side and the quick menu travel with
// the calls.
//
// NGB_D3D_THREAD=capture: each call recorded and drawn from the recording at once, on the title's thread -- the
// renderer then draws from nothing but what was recorded (a test of the recording; the default draws directly).
#include <algorithm>
#include <cstring>
#include <deque>
#include <unordered_map>
#include <vector>

#include "gpu/backend.h"
#include "gpu/headers.h"
#include "kernel/kernel.h"

extern "C" uint32_t xbox_HeapAlloc(uint32_t size, uint32_t align);

namespace gpu {

namespace {

// commands handed over at once
constexpr uint32_t kBatchCommands = 64;
// guest memory for what the renderer reads through a guest address -- the vertices of a DrawVerticesUP, a copy's
// rects -- copied at the call: the title may reuse its own once the call returns
constexpr uint32_t kScratchRing = 4u << 20;

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

struct Batch {
    std::vector<Cmd> cmds;
    std::vector<uint8_t> bytes;
    std::vector<std::shared_ptr<VertexShader>> vs;
    std::vector<std::shared_ptr<PixelShader>> ps;
    std::vector<QuickPeek> quick;
};

// The plain data in State around its shader pointers, sent as it changes in 64 byte pieces. Of the transforms only
// those drawn with: view and projection (2, 3), the textures' (16-19) and the world matrices (256-259).
struct Range {
    size_t off, len;
};
size_t At(const State& s, const void* p) {
    return size_t(static_cast<const uint8_t*>(p) - reinterpret_cast<const uint8_t*>(&s));
}
std::vector<Range> StateRanges() {
    static const State s;
    return {
        {At(s, &s.simple), At(s, &s.index_buffer) - At(s, &s.simple)},   // render states, textures, the streams
        {At(s, &s.index_buffer), At(s, &s.vs) - At(s, &s.index_buffer)},
        {At(s, &s.vsc), At(s, &s.transforms) - At(s, &s.vsc)},            // constants, viewport, surfaces, scissors
        {At(s, &s.transforms[2]), 2 * sizeof(s.transforms[0])},
        {At(s, &s.transforms[16]), 4 * sizeof(s.transforms[0])},
        {At(s, &s.transforms[256]), 4 * sizeof(s.transforms[0])},
        {At(s, &s.material), sizeof(State) - At(s, &s.material)},
    };
}

constexpr size_t kPiece = 64;

class Recorded final : public Backend {
public:
    explicit Recorded(std::unique_ptr<Backend> inner) : inner_(std::move(inner)), ranges_(StateRanges()) {
        scratch_ = xbox_HeapAlloc(kScratchRing, 64);
    }

    bool Init(uint32_t width, uint32_t height) override { return inner_->Init(width, height); }

    void Draw(const State& s, uint32_t prim, uint32_t start, uint32_t count, const uint8_t* indices) override {
        SendState(s, start + count);
        Cmd c{Op::Draw};
        c.a = prim;
        c.b = start;
        c.c = count;
        if (indices) {
            c.d = 1;
            c.data = Put(indices, size_t(count) * 2);
        }
        cur_.cmds.push_back(c);
        Submit(false);
    }

    void Clear(const State& s, uint32_t count, const uint8_t* rects, uint32_t flags, uint32_t color, float z,
               uint32_t stencil) override {
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
        cur_.cmds.push_back(c);
        Submit(false);
    }

    // rects and points are taken now; the copy itself runs in order
    void Copy(const Surface& src, const Surface& dst, uint32_t count, uint32_t src_rects, uint32_t dst_points) override {
        SendHeader(src.header);
        SendHeader(dst.header);
        FlushHeaders();
        Cmd c{Op::Copy};
        c.a = count;
        c.b = count && src_rects ? Scratch(kernel::HostPtr(src_rects), count * 16) : 0;
        c.c = count && dst_points ? Scratch(kernel::HostPtr(dst_points), count * 8) : 0;
        c.s1 = src;
        c.s2 = dst;
        cur_.cmds.push_back(c);
        Submit(false);
    }

    void Present(const Surface& front) override {
        SendHeader(front.header);
        FlushHeaders();
        SendPeek(sent_);
        Cmd c{Op::Present};
        c.s1 = front;
        c.quick = int32_t(cur_.quick.size());
        cur_.quick.emplace_back();
        inner_->CaptureQuick(cur_.quick.back());
        cur_.cmds.push_back(c);
        Submit(true);
    }

    void Sync(uint32_t data, uint32_t size) override {
        Submit(true);
        inner_->Sync(data, size);
    }
    void SyncLate(uint32_t data) override {
        Submit(true);
        inner_->SyncLate(data);
    }
    void Flush() override {
        cur_.cmds.push_back(Cmd{Op::Flush});
        Submit(true);
    }
    void SetGamma(const uint8_t* ramp) override {
        Cmd c{Op::Gamma};
        c.data = Put(ramp, 768);
        cur_.cmds.push_back(c);
    }
    void VisibilityBegin() override { cur_.cmds.push_back(Cmd{Op::VisBegin}); }
    void VisibilityEnd(uint32_t report) override {
        Cmd c{Op::VisEnd};
        c.a = report;
        cur_.cmds.push_back(c);
    }

private:
    uint32_t Put(const void* p, size_t n) {
        size_t at = (cur_.bytes.size() + 3) & ~size_t(3);
        cur_.bytes.resize(at + n);
        std::memcpy(cur_.bytes.data() + at, p, n);
        return uint32_t(at);
    }

    // bytes the renderer reads through a guest address, copied into the scratch ring
    uint32_t Scratch(const void* p, uint32_t n) {
        uint32_t size = (n + 63) & ~63u;
        if (!scratch_ || size > kScratchRing) return 0;
        if (scratch_pos_ + size > kScratchRing) scratch_pos_ = 0;
        uint32_t at = scratch_ + scratch_pos_;
        scratch_pos_ += size;
        std::memcpy(kernel::HostPtr(at), p, n);
        return at;
    }

    // a header as it is now, to the renderer's copies when it changed since it was last sent
    void SendHeader(uint32_t header, HeaderCopy* out = nullptr) {
        if (!header) return;
        HeaderCopy now{};
        // vertex buffer, index buffer and palette headers are 12 bytes: reading 20 can run off the end of the heap
        uint32_t type = kernel::R32(header) & 0x70000;   // 0 vertex, 0x10000 index, 0x30000 palette
        std::memcpy(now.w, kernel::HostPtr(header), type < 0x20000 || type == 0x30000 ? 12 : 20);
        if (out) *out = now;
        auto it = sent_headers_.find(header);
        if (it != sent_headers_.end() && !std::memcmp(it->second.w, now.w, sizeof now.w)) return;
        sent_headers_[header] = now;
        pending_headers_.push_back({header, now});
    }
    // a vertex buffer of the renderer's own, its data in the title's memory (DrawVerticesUP: Common bit 31): the
    // vertices copied, the copy's header pointing at them
    void SendUpHeader(uint32_t header, uint32_t bytes) {
        HeaderCopy now{};
        std::memcpy(now.w, kernel::HostPtr(header), 12);
        now.w[1] = Scratch(kernel::HostPtr(now.w[1]), bytes);
        sent_headers_[header] = now;
        pending_headers_.push_back({header, now});
    }
    void FlushHeaders() {
        if (pending_headers_.empty()) return;
        Cmd c{Op::Headers};
        c.data = Put(pending_headers_.data(), pending_headers_.size() * sizeof(pending_headers_[0]));
        c.size = uint32_t(pending_headers_.size());
        cur_.cmds.push_back(c);
        pending_headers_.clear();
    }

    // the game's side of the call, when it changed
    void SendPeek(const State& s) {
        Peek p;
        inner_->CapturePeek(s, p);
        if (peek_sent_ && !std::memcmp(&p, &sent_peek_, sizeof p)) return;
        peek_sent_ = true;
        sent_peek_ = p;
        Cmd c{Op::Peek};
        c.data = Put(&p, sizeof p);
        cur_.cmds.push_back(c);
    }

    // a field of the state as the renderer has it, when it differs from what was sent
    void Delta(const void* field, const void* value, size_t n) {
        uint8_t* sent = reinterpret_cast<uint8_t*>(&sent_) + At(sent_, field);
        if (!std::memcmp(sent, value, n)) return;
        std::memcpy(sent, value, n);
        uint32_t head[2] = {uint32_t(At(sent_, field)), uint32_t(n)};
        size_t at = cur_.bytes.size();
        cur_.bytes.resize(at + 8 + n);
        std::memcpy(cur_.bytes.data() + at, head, 8);
        std::memcpy(cur_.bytes.data() + at + 8, value, n);
    }

    // what changed since the last command, in 64 byte pieces; the headers the call names; the game's side.
    // vertices: how many a non-indexed draw reads (a DrawVerticesUP's to copy)
    void SendState(const State& s, uint32_t vertices) {
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
        size_t first = cur_.bytes.size();
        const uint8_t* sent = reinterpret_cast<const uint8_t*>(&sent_);
        for (const Range& r : ranges_) {
            size_t off = r.off, end = r.off + r.len;
            for (; off + kPiece <= end; off += kPiece)
                if (std::memcmp(sent + off, now + off, kPiece)) Delta(sent + off, now + off, kPiece);
            if (off < end) Delta(sent + off, now + off, end - off);
        }
        if (s.vs.get() != sent_.vs.get()) {
            sent_.vs = s.vs;
            c.vs = int32_t(cur_.vs.size());
            cur_.vs.push_back(s.vs);
        }
        if (s.ps.get() != sent_.ps.get()) {
            sent_.ps = s.ps;
            c.ps = int32_t(cur_.ps.size());
            cur_.ps.push_back(s.ps);
        }
        if (cur_.bytes.size() == first && c.vs < 0 && c.ps < 0) return;
        c.data = uint32_t(first);
        c.size = uint32_t(cur_.bytes.size() - first);
        cur_.cmds.push_back(c);
    }

    // the batch drawn: at once (NGB_D3D_THREAD=capture), after every call
    void Submit(bool) {
        if (cur_.cmds.empty()) return;
        const auto* was = g_header_view;
        g_header_view = &headers_;
        for (const Cmd& c : cur_.cmds) Execute(cur_, c);
        g_header_view = was;
        cur_.cmds.clear();
        cur_.bytes.clear();
        cur_.vs.clear();
        cur_.ps.clear();
        cur_.quick.clear();
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
                const auto* h = reinterpret_cast<const std::pair<uint32_t, HeaderCopy>*>(data);
                for (uint32_t i = 0; i < c.size; i++) headers_[h[i].first] = h[i].second;
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
            case Op::Copy: inner_->Copy(c.s1, c.s2, c.a, c.b, c.c); break;
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

    // the title's side
    Batch cur_;
    State sent_;   // the state as the renderer will have it
    std::unordered_map<uint32_t, HeaderCopy> sent_headers_;
    std::vector<std::pair<uint32_t, HeaderCopy>> pending_headers_;
    Peek sent_peek_;
    bool peek_sent_ = false;
    uint32_t scratch_ = 0, scratch_pos_ = 0;

    // the renderer's side
    State state_;
    std::unordered_map<uint32_t, HeaderCopy> headers_;
};

}  // namespace

std::unique_ptr<Backend> CreateThreadedBackend(std::unique_ptr<Backend> inner) {
    return std::make_unique<Recorded>(std::move(inner));
}

}  // namespace gpu
