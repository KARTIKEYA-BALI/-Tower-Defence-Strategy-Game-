#include <stdlib.h>
#include "spell.h"
#include "queue.h"

int spell_area(const Grid *g,int centre,int radius,int out[],int max_len){
    int end_cell,neighbours[4],num_neigh,count=0;
    int *depth;
    Queue q;
    Event e={0};
    if(g==NULL || g->cells==NULL || out==NULL || max_len<1) return 0;
    if(radius<0 || radius>SPELL_MAX_RADIUS) return 0;
    end_cell=g->width*g->height;
    if(centre<0 || centre>=end_cell) return 0;
    if(g->cells[centre].terrain==CELL_BLOCKED) return 0;

    depth=(int *)malloc(end_cell*sizeof(int));
    if(depth==NULL) return 0;
    for(int i=0;i<end_cell;i++) depth[i]=-1;
    if(!queue_init(&q,end_cell)){
        free(depth);
        return 0;
    }

    depth[centre]=0;
    out[count++]=centre;
    e.cell=centre;
    queue_push(&q,e);
    while(queue_pop(&q,&e)){
        int c=e.cell;
        if(depth[c]==radius) continue;
        num_neigh=grid_neighbours(g,c,neighbours);
        for(int k=0;k<num_neigh;k++){
            int nb=neighbours[k];
            if(depth[nb]!=-1) continue;
            if(!grid_is_walkable(g,nb%g->width,nb/g->width)) continue;
            if(count>=max_len){
                queue_free(&q);
                free(depth);
                return count;
            }
            depth[nb]=depth[c]+1;
            out[count++]=nb;
            e.cell=nb;
            queue_push(&q,e);
        }
    }
    queue_free(&q);
    free(depth);
    return count;
}

void zones_clear(ZoneTable *zt){
    if(zt==NULL) return;
    for(int i=0;i<ZONE_MAX;i++) zt->zones[i].active=0;
    for(int k=0;k<ZONE_KIND_COUNT;k++){
        for(int c=0;c<ZONE_GRID_CELLS;c++) zt->cover[k][c]=0;
    }
    zt->active_count=0;
    heap_init(&zt->expiry);
}

int zones_add(ZoneTable *zt,int kind,const int cells[],int count,int expires){
    int slot=-1;
    if(zt==NULL || cells==NULL) return -1;
    if(kind<0 || kind>=ZONE_KIND_COUNT || count<1 || count>ZONE_MAX_CELLS) return -1;
    for(int i=0;i<count;i++){
        if(cells[i]<0 || cells[i]>=ZONE_GRID_CELLS) return -1;
    }
    for(int i=0;i<ZONE_MAX;i++){
        if(!zt->zones[i].active){
            slot=i;
            break;
        }
    }
    if(slot==-1) return -1;
    if(!heap_push(&zt->expiry,expires,slot)) return -1;
    Zone *z=&zt->zones[slot];
    z->kind=kind;
    z->count=count;
    z->expires=expires;
    z->active=1;
    for(int i=0;i<count;i++){
        z->cells[i]=cells[i];
        zt->cover[kind][cells[i]]++;
    }
    zt->active_count++;
    return slot;
}

int zones_update(ZoneTable *zt,int now){
    HeapItem item;
    int removed=0;
    if(zt==NULL) return 0;
    while(!heap_is_empty(&zt->expiry) && zt->expiry.items[0].priority<=now){
        heap_pop(&zt->expiry,&item);
        Zone *z=&zt->zones[item.value];
        if(!z->active) continue;
        for(int i=0;i<z->count;i++){
            if(zt->cover[z->kind][z->cells[i]]>0) zt->cover[z->kind][z->cells[i]]--;
        }
        z->active=0;
        zt->active_count--;
        removed++;
    }
    return removed;
}

int zones_cover(const ZoneTable *zt,int kind,int cell){
    if(zt==NULL || kind<0 || kind>=ZONE_KIND_COUNT || cell<0 || cell>=ZONE_GRID_CELLS) return 0;
    return zt->cover[kind][cell];
}

int zones_until(const ZoneTable *zt,int kind,int cell){
    int latest=-1;
    if(zones_cover(zt,kind,cell)==0) return -1;
    for(int i=0;i<ZONE_MAX;i++){
        const Zone *z=&zt->zones[i];
        if(!z->active || z->kind!=kind || z->expires<=latest) continue;
        for(int j=0;j<z->count;j++){
            if(z->cells[j]==cell){
                latest=z->expires;
                break;
            }
        }
    }
    return latest;
}

int zones_active(const ZoneTable *zt){
    return zt ? zt->active_count : 0;
}
