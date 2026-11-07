package io.github.gamboozino.ngbrecomp;
import java.util.Locale;

public final class QuickPanelTest {
    static void check(boolean value) { if (!value) throw new AssertionError(); }
    static void sameRegions(QuickPanelLayout a,QuickPanelLayout b) {
        check(a.regions.size()==3 && b.regions.size()==3);
        for(int i=0;i<3;i++) {
            QuickPanelLayout.Region x=a.regions.get(i),y=b.regions.get(i);
            check(x.category==y.category && x.x==y.x && x.y==y.y && x.w==y.w && x.h==y.h);
        }
    }
    public static void main(String[] args) {
        String packet = "7\t2\t1\n1\t201\t3\t1\t0\t3\t1\tskata.spr\t真龙剑\n" +
            "4\t190\t2\t0\t1\t0\t0\tre_hp_s.spr\t灵命药\n";
        QuickPanelState state = QuickPanelState.parse(packet);
        check(state.epoch == 7 && state.mode == 2 && state.freeze);
        check(state.category(1).get(0).equipped && state.category(1).get(0).name.equals("真龙剑"));
        check(!state.category(4).get(0).usable && state.category(4).get(0).quantity);
        check(QuickPanelState.parse("broken").mode == 1);
        check(QuickPanelState.parse("7\t2\t1\n7\t2\t0\t1\t0\t0\t0\tx\tx\n").entries.isEmpty());
        check(QuickPanelState.parse("7\t2\t1\n6\t80\t1\t1\t0\t0\t0\tpanel_armlet.spr\t静心腕轮\n").category(6).size()==1);
        QuickPanelState costumes = QuickPanelState.parse("7\t2\t1\n5\t255\t1\t1\t0\t0\t1\tcos_menu.spr#1\tDefault Costume\n");
        check(costumes.category(5).size() == 1 && costumes.category(5).get(0).name.equals("剧情默认"));
        QuickPanelEvents events = new QuickPanelEvents();
        events.add(7, 1, 4, 190); events.add(7, 2, 4, 190);
        check(events.poll() == (7L << 32 | 1L << 24 | 4L << 16 | 190));
        check(events.poll() == (7L << 32 | 2L << 24 | 4L << 16 | 190));
        check(events.poll() == 0);
        for (int i = 0; i < 33; i++) events.add(7, 1, 1, 201);
        check(events.poll() == (7L << 32 | 3L << 24)); check(events.poll() == 0);
        QuickPanelState inventory = new QuickPanelState(); inventory.epoch = 1; inventory.mode = 2;
        int[] counts = {14, 9, 4, 6, 9, 14};
        for (int c = 1; c <= 6; c++) for (int i = 0; i < counts[c-1]; i++) {
            QuickPanelState.Entry entry = new QuickPanelState.Entry(); entry.category = c;
            entry.id = c * 100 + i; inventory.entries.add(entry);
        }
        for (int[] screen : new int[][] {{1240,1080}, {1080,1240}, {1920,1080}, {720,480}}) {
            float density = screen[0] == 720 ? 1 : 369f/160;
          for (boolean expanded : new boolean[]{false,true}) {
            QuickPanelLayout layout = new QuickPanelLayout(inventory, screen[0], screen[1], density,expanded);
            check(layout.cells.size() == (expanded ? 29 : 31));
            check(layout.height==screen[1] && layout.toggle!=null);
            QuickPanelLayout.Region footer=layout.regions.get(2);
            check(layout.toggle.w==layout.mapButton.w && layout.toggle.h==52*density);
            check(layout.mapButton.x==footer.x);
            check(Math.abs(layout.toggle.x+layout.toggle.w-footer.x-footer.w)<.001f);
            check(Math.abs(layout.toggle.y-(screen[1]-58*density))<.001f);
            QuickPanelLayout otherPage=new QuickPanelLayout(inventory,screen[0],screen[1],density,!expanded);
            check(layout.toggle.sameBounds(otherPage.toggle));
            check(layout.mapButton.sameBounds(otherPage.mapButton));
            for (QuickPanelLayout.Cell cell : layout.cells) {
                check(cell.x >= 0 && cell.y >= 0 && cell.x + cell.w <= screen[0] + 0.01f);
                check(cell.h >= 44 * density && cell.w>=44*density && cell.y + cell.h <= layout.height);
                check(cell.entry==null || (expanded ? cell.category()==3 || cell.category()==5 || cell.category()==6 :
                    cell.category()==1 || cell.category()==2 || cell.category()==4));
                check(layout.hit(cell.x+cell.w/2,cell.y+cell.h/2)==cell);
            }
            for (int i = 0; i < layout.cells.size(); i++) for (int j = i+1; j < layout.cells.size(); j++) {
                QuickPanelLayout.Cell a = layout.cells.get(i), b = layout.cells.get(j);
                check(a.x+a.w <= b.x+0.01f || b.x+b.w <= a.x+0.01f || a.y+a.h <= b.y+0.01f || b.y+b.h <= a.y+0.01f);
            }
            System.out.printf(Locale.ROOT,"layout %dx%d %s: %d buttons, no scroll%n",screen[0],screen[1],
                expanded ? "drawer" : "main",layout.cells.size());
          }
        }
        QuickPanelUi ui=new QuickPanelUi();
        check(!ui.expanded); ui.toggle(); check(ui.expanded);
        check(!ui.selected(1) && ui.expanded);
        for(int c:new int[]{3,6,5}) {
            check(ui.selected(c) && !ui.expanded); ui.toggle();
        }
        ui.reset(); check(!ui.expanded);
        check(!ui.selected(6));
        QuickPanelState map=QuickPanelState.parse("7\t2\t1\t49\tsatoallj.spr\t1\t0.25\t0.75\n");
        check(map.mapId==49 && map.mapPosition && map.mapX==.25f && map.mapY==.75f);
        check(QuickPanelState.parse("7\t2\t1\t-1\t-\t0\t0\t0\n").mapId==-1);
        check(QuickPanelState.parse("7\t2\t1\t52\tx\t0\t0\t0\n").error!=null);
        check(QuickPanelState.parse("7\t2\t1\t0\tx\t1\tNaN\t0\n").error!=null);
        ui.toggleMap(false); check(!ui.mapOpen);
        ui.toggle(); ui.toggleMap(true); check(ui.mapOpen && !ui.expanded);
        ui.toggleMap(true); check(!ui.mapOpen);
        ui.toggleMap(true); ui.toggle(); check(!ui.mapOpen && ui.expanded);
        ui.reset(); check(!ui.mapOpen && !ui.expanded);
        QuickPanelState sparse=new QuickPanelState(); sparse.mode=2;
        for(int[] screen:new int[][]{{1240,1080},{720,480}}) {
            for(boolean expanded:new boolean[]{false,true}) {
                QuickPanelLayout empty=new QuickPanelLayout(sparse,screen[0],screen[1],1,expanded);
                check(empty.cells.size()==2 && empty.toggle==empty.cells.get(0));
                sameRegions(empty,new QuickPanelLayout(inventory,screen[0],screen[1],1,expanded));
            }
        }
        for(int limit:new int[]{1,3,7}) {
            QuickPanelState partial=new QuickPanelState();partial.mode=2;
            for(QuickPanelState.Entry e:inventory.entries) if(e.id%100<limit) partial.entries.add(e);
            for(boolean expanded:new boolean[]{false,true}) {
                QuickPanelLayout layout=new QuickPanelLayout(partial,1240,1080,369f/160,expanded);
                QuickPanelLayout full=new QuickPanelLayout(inventory,1240,1080,369f/160,expanded);
                check(layout.toggle.sameBounds(full.toggle));
                sameRegions(layout,full);
                for(QuickPanelLayout.Cell c:layout.cells) {
                    check(c.w>=44*(369f/160) && c.h>=44*(369f/160));
                    check(c.x+c.w<=1240 && c.y+c.h<=1080);
                }
            }
        }
        System.out.println("quick panel Java checks passed");
    }
}
