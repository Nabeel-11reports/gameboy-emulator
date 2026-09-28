#include "cpu.h"
#include <stdio.h>

#define FLAG_Z 0x80u
#define FLAG_N 0x40u
#define FLAG_H 0x20u
#define FLAG_C 0x10u

static uint8_t rd8(CPU *c, Bus *b) { (void)c; return bus_read8(b, c->pc++); }
static uint16_t rd16(CPU *c, Bus *b) { uint8_t lo = rd8(c,b); uint8_t hi = rd8(c,b); return make16(hi,lo); }
static uint8_t r8(CPU *c, Bus *b, int r) {
    switch (r & 7) {
        case 0: return c->b; case 1: return c->c; case 2: return c->d; case 3: return c->e;
        case 4: return c->h; case 5: return c->l; case 6: return bus_read8(b, make16(c->h,c->l)); default: return c->a;
    }
}
static void w8(CPU *c, Bus *b, int r, uint8_t v) {
    switch (r & 7) {
        case 0: c->b=v; break; case 1: c->c=v; break; case 2: c->d=v; break; case 3: c->e=v; break;
        case 4: c->h=v; break; case 5: c->l=v; break; case 6: bus_write8(b,make16(c->h,c->l),v); break; default: c->a=v; break;
    }
}
static uint16_t rr(CPU *c, int p) {
    switch (p & 3) {
        case 0: return make16(c->b,c->c); case 1: return make16(c->d,c->e); case 2: return make16(c->h,c->l); default: return c->sp;
    }
}
static void wr(CPU *c, int p, uint16_t v) {
    switch (p & 3) {
        case 0: c->b=(uint8_t)(v>>8); c->c=(uint8_t)v; break;
        case 1: c->d=(uint8_t)(v>>8); c->e=(uint8_t)v; break;
        case 2: c->h=(uint8_t)(v>>8); c->l=(uint8_t)v; break;
        default: c->sp=v; break;
    }
}
static uint16_t rr2(CPU *c, int p) {
    switch (p & 3) {
        case 0: return make16(c->b,c->c); case 1: return make16(c->d,c->e); case 2: return make16(c->h,c->l);
        default: return make16(c->a,c->f);
    }
}
static void wr2(CPU *c, int p, uint16_t v) {
    switch (p & 3) {
        case 0: c->b=(uint8_t)(v>>8); c->c=(uint8_t)v; break;
        case 1: c->d=(uint8_t)(v>>8); c->e=(uint8_t)v; break;
        case 2: c->h=(uint8_t)(v>>8); c->l=(uint8_t)v; break;
        default: c->a=(uint8_t)(v>>8); c->f=(uint8_t)v & 0xF0u; break;
    }
}
static void push16(CPU *c, Bus *b, uint16_t v) {
    c->sp--; bus_write8(b,c->sp,(uint8_t)(v>>8));
    c->sp--; bus_write8(b,c->sp,(uint8_t)v);
}
static uint16_t pop16(CPU *c, Bus *b) {
    uint8_t lo=bus_read8(b,c->sp++); uint8_t hi=bus_read8(b,c->sp++); return make16(hi,lo);
}
static void setz(CPU *c, bool on) { if (on) c->f |= FLAG_Z; else c->f &= (uint8_t)~FLAG_Z; }
static void setn(CPU *c, bool on) { if (on) c->f |= FLAG_N; else c->f &= (uint8_t)~FLAG_N; }
static void seth(CPU *c, bool on) { if (on) c->f |= FLAG_H; else c->f &= (uint8_t)~FLAG_H; }
static void setc(CPU *c, bool on) { if (on) c->f |= FLAG_C; else c->f &= (uint8_t)~FLAG_C; }
static bool getz(const CPU *c) { return (c->f & FLAG_Z)!=0; }
static bool getcarry(const CPU *c) { return (c->f & FLAG_C)!=0; }

static bool getn(const CPU *c) { return (c->f & FLAG_N)!=0; }

static uint8_t inc8(CPU *c, uint8_t x) {
    uint8_t r=(uint8_t)(x+1); setz(c,r==0); setn(c,false); seth(c,(x&0x0F)==0x0F); return r;
}
static uint8_t dec8(CPU *c, uint8_t x) {
    uint8_t r=(uint8_t)(x-1); setz(c,r==0); setn(c,true); seth(c,(x&0x0F)==0); return r;
}
static void add_a(CPU *c, uint8_t v, bool with_carry) {
    uint8_t cin=with_carry && getcarry(c); uint16_t r=(uint16_t)c->a+v+cin; setz(c,(uint8_t)r==0); setn(c,false);
    seth(c,((c->a&0x0F)+(v&0x0F)+cin)>0x0F); setc(c,r>0xFF); c->a=(uint8_t)r;
}
static void sub_a(CPU *c, uint8_t v, bool with_borrow) {
    uint8_t bin=with_borrow && getcarry(c); int r=(int)c->a-v-bin; setz(c,(uint8_t)r==0); setn(c,true);
    seth(c,(int)(c->a&0x0F)-(int)(v&0x0F)-bin<0); setc(c,r<0); c->a=(uint8_t)r;
}
static void cp_a(CPU *c, uint8_t v) {
    int r=(int)c->a-v; setz(c,(uint8_t)r==0); setn(c,true); seth(c,(int)(c->a&0x0F)-(int)(v&0x0F)<0); setc(c,r<0);
}
static uint8_t alu_cb(CPU *c, int op, uint8_t v) {
    bool carry=false;
    switch (op) {
        case 0: carry=(v&0x80)!=0; v=(uint8_t)((v<<1)|(v>>7)); break;
        case 1: carry=(v&1)!=0; v=(uint8_t)((v>>1)|(v<<7)); break;
        case 2: { bool old=getcarry(c); carry=(v&0x80)!=0; v=(uint8_t)((v<<1)|(old?1:0)); } break;
        case 3: { bool old=getcarry(c); carry=(v&1)!=0; v=(uint8_t)((v>>1)|(old?0x80:0)); } break;
        case 4: carry=(v&0x80)!=0; v<<=1; break;
        case 5: carry=(v&1)!=0; v=(uint8_t)((v>>1)|(v&0x80)); break;
        case 6: v=(uint8_t)((v<<4)|(v>>4)); carry=false; break;
        default: carry=(v&1)!=0; v>>=1; break;
    }
    setz(c,v==0); setn(c,false); seth(c,false); setc(c,carry); return v;
}
static void daa(CPU *c) {
    int a=c->a; bool carry=getcarry(c);
    if (!getn(c)) {
        if ((c->f & FLAG_H) || (a & 0x0F)>9) a += 0x06;
        if (carry || a>0x9F) { a += 0x60; carry=true; }
    } else {
        if (c->f & FLAG_H) a = (a-6)&0xFF;
        if (carry) a -= 0x60;
    }
    c->a=(uint8_t)a; setz(c,c->a==0); seth(c,false); setc(c,carry);
}
static void add_hl(CPU *c, uint16_t v) {
    uint16_t hl=make16(c->h,c->l); uint32_t r=(uint32_t)hl+v;
    setn(c,false); seth(c,((hl&0x0FFFu)+(v&0x0FFFu))>0x0FFFu); setc(c,r>0xFFFFu); wr(c,2,(uint16_t)r);
}
static uint16_t add_sp_e8(CPU *c, int8_t e) {
    uint16_t sp=c->sp; uint16_t u=(uint16_t)(uint8_t)e; uint16_t r=(uint16_t)(sp + e);
    setz(c,false); setn(c,false); seth(c,((sp&0x0F)+(u&0x0F))>0x0F); setc(c,((sp&0xFF)+(u&0xFF))>0xFF); return r;
}
static void interrupt(CPU *c, Bus *b, int bit, uint16_t vector) {
    c->halted=false; c->ime=false; b->mem[0xFF0F] &= (uint8_t)~(1u<<bit); push16(c,b,c->pc); c->pc=vector;
}

void cpu_init(CPU *cpu) {
    cpu->a=0x01; cpu->f=0xB0; cpu->b=0x00; cpu->c=0x13; cpu->d=0x00; cpu->e=0xD8; cpu->h=0x01; cpu->l=0x4D;
    cpu->sp=0xFFFE; cpu->pc=0x0100; cpu->ime=false; cpu->halted=false; cpu->stopped=false; cpu->ei_delay=0;
}

static int handle_interrupts(CPU *c, Bus *b) {
    uint8_t pending=(uint8_t)(b->mem[0xFF0F] & b->mem[0xFFFF] & 0x1Fu);
    if (!pending) return 0;
    if (c->halted) c->halted=false;
    if (!c->ime) return 0;
    for (int bit=0; bit<5; ++bit) if (pending & (1u<<bit)) {
        interrupt(c,b,bit,(uint16_t)(0x40 + bit*8));
        return 20;
    }
    return 0;
}

int cpu_step(CPU *c, Bus *b) {
    int irq=handle_interrupts(c,b); if (irq) return irq;
    if (c->stopped) return 4;
    if (c->halted) return 4;

    uint8_t op=rd8(c,b);
    int x=op>>6, y=(op>>3)&7, z=op&7;
    int cycles=4;

    if (x==0) {
        switch(z) {
            case 0:
                if (y==0) { cycles=4; }
                else if (y==1) { (void)rd8(c,b); c->stopped=true; cycles=4; }
                else if (y==2) {
                    int8_t e=(int8_t)rd8(c,b);
                    c->pc=(uint16_t)(c->pc+e);
                    cycles=12;
                } else {
                    bool cond=(y==3)?!getz(c):(y==4)?getz(c):(y==5)?!getcarry(c):getcarry(c);
                    int8_t e=(int8_t)rd8(c,b);
                    if(cond) { c->pc=(uint16_t)(c->pc+e); cycles=12; } else cycles=8;
                }
                break;
            case 1:
                if ((y&1)==0) { wr(c,y>>1,rd16(c,b)); cycles=12; }
                else { add_hl(c,rr(c,y>>1)); cycles=8; }
                break;
            case 2:
                {
                    int p=y>>1;
                    bool load=(y&1)!=0;
                    uint16_t addr;
                    if (p==0) addr=make16(c->b,c->c);
                    else if (p==1) addr=make16(c->d,c->e);
                    else addr=make16(c->h,c->l);
                    if (load) c->a=bus_read8(b,addr);
                    else bus_write8(b,addr,c->a);
                    if (p==2 || p==3) {
                        int delta=(p==2)?1:-1;
                        addr=(uint16_t)(addr+delta);
                        c->h=(uint8_t)(addr>>8);
                        c->l=(uint8_t)addr;
                    }
                    cycles=8;
                }
                break;
            case 3:
                { int p=y>>1; uint16_t v=rr(c,p); v=(uint16_t)(v+((y&1)?-1:1)); wr(c,p,v); cycles=8; }
                break;
            case 4: { uint8_t v=r8(c,b,y); v=inc8(c,v); w8(c,b,y,v); cycles=(y==6)?12:4; break; }
            case 5: { uint8_t v=r8(c,b,y); v=dec8(c,v); w8(c,b,y,v); cycles=(y==6)?12:4; break; }
            case 6: { uint8_t v=rd8(c,b); w8(c,b,y,v); cycles=(y==6)?12:8; break; }
            case 7:
                { bool carry; switch(y){
                    case 0: carry=(c->a&0x80)!=0; c->a=(uint8_t)((c->a<<1)|(c->a>>7)); break;
                    case 1: carry=(c->a&1)!=0; c->a=(uint8_t)((c->a>>1)|(c->a<<7)); break;
                    case 2: { bool old=getcarry(c); carry=(c->a&0x80)!=0; c->a=(uint8_t)((c->a<<1)|(old?1:0)); } break;
                    case 3: { bool old=getcarry(c); carry=(c->a&1)!=0; c->a=(uint8_t)((c->a>>1)|(old?0x80:0)); } break;
                    case 4: daa(c); carry=false; break;
                    case 5: c->a=(uint8_t)~c->a; setn(c,true); seth(c,true); carry=getcarry(c); break;
                    case 6: setn(c,false); seth(c,false); setc(c,true); carry=true; break;
                    default: setn(c,false); seth(c,false); setc(c,!getcarry(c)); carry=getcarry(c); break;
                } if(y<=3){ setz(c,false); setn(c,false); seth(c,false); setc(c,carry);} cycles=4; }
                break;
        }
    } else if (x==1) {
        if (op==0x76) { c->halted=true; cycles=4; }
        else { uint8_t v=r8(c,b,z); w8(c,b,y,v); cycles=(y==6||z==6)?8:4; }
    } else if (x==2) {
        uint8_t v=r8(c,b,z);
        switch(y){ case 0:add_a(c,v,false);break; case 1:add_a(c,v,true);break; case 2:sub_a(c,v,false);break; case 3:sub_a(c,v,true);break; case 4:c->a&=v;setz(c,c->a==0);setn(c,false);seth(c,true);setc(c,false);break; case 5:c->a^=v;setz(c,c->a==0);setn(c,false);seth(c,false);setc(c,false);break; case 6:c->a|=v;setz(c,c->a==0);setn(c,false);seth(c,false);setc(c,false);break; default:cp_a(c,v);break; }
        cycles=(z==6)?8:4;
    } else {
        switch(z) {
            case 0:
                if (y<4) { bool cond=(y==0)?!getz(c):(y==1)?getz(c):(y==2)?!getcarry(c):getcarry(c); uint16_t ret=pop16(c,b); if(cond)c->pc=ret; cycles=cond?20:8; }
                else if (y==4) { uint8_t a8=rd8(c,b); bus_write8(b,(uint16_t)(0xFF00u+a8),c->a); cycles=12; }
                else if (y==5) { int8_t e=(int8_t)rd8(c,b); c->sp=add_sp_e8(c,e); cycles=16; }
                else if (y==6) { uint8_t a8=rd8(c,b); c->a=bus_read8(b,(uint16_t)(0xFF00u+a8)); cycles=12; }
                else { int8_t e=(int8_t)rd8(c,b); wr(c,2,add_sp_e8(c,e)); cycles=12; }
                break;
            case 1:
                if ((y&1)==0) { wr2(c,y>>1,pop16(c,b)); cycles=12; }
                else if (y==1) { c->pc=pop16(c,b); cycles=16; }
                else if (y==3) { c->ime=true; c->pc=pop16(c,b); cycles=16; }
                else if (y==5) { c->pc=make16(c->h,c->l); cycles=4; }
                else { c->sp=make16(c->h,c->l); cycles=8; }
                break;
            case 2:
                if (y<4) { uint16_t a=rd16(c,b); bool cond=(y==0)?!getz(c):(y==1)?getz(c):(y==2)?!getcarry(c):getcarry(c); if(cond)c->pc=a; cycles=cond?16:12; }
                else if (y==4) { uint16_t a=rd16(c,b); bus_write8(b,a,c->a); cycles=16; }
                else if (y==6) { bus_write8(b,(uint16_t)(0xFF00u+c->c),c->a); cycles=8; }
                else if (y==7) { c->a=bus_read8(b,(uint16_t)(0xFF00u+c->c)); cycles=8; }
                break;
            case 3:
                if (y==0) { c->pc=rd16(c,b); cycles=16; }
                else if (y==1) { uint8_t cb=rd8(c,b); int bx=cb>>6, by=(cb>>3)&7, bz=cb&7; if(bx==0){uint8_t v=r8(c,b,bz);v=alu_cb(c,by,v);w8(c,b,bz,v);cycles=(bz==6)?16:8;} else if(bx==1){uint8_t v=r8(c,b,bz);setz(c,(v&(1u<<by))==0);setn(c,false);seth(c,true);cycles=(bz==6)?12:8;} else {uint8_t v=r8(c,b,bz);if(bx==2)v&=(uint8_t)~(1u<<by);else v|=(uint8_t)(1u<<by);w8(c,b,bz,v);cycles=(bz==6)?16:8;} }
                else if (y==2) { c->ime=false; cycles=4; }
                else if (y==3) { c->ime=true; c->ei_delay=2; cycles=4; }
                break;
            case 4:
                if (y<4) { uint16_t a=rd16(c,b); bool cond=(y==0)?!getz(c):(y==1)?getz(c):(y==2)?!getcarry(c):getcarry(c); if(cond){push16(c,b,c->pc);c->pc=a;cycles=24;}else cycles=12; }
                break;
            case 5:
                if ((y&1)==0) { push16(c,b,rr2(c,y>>1)); cycles=16; }
                else if (y==1) { uint16_t a=rd16(c,b); push16(c,b,c->pc); c->pc=a; cycles=24; }
                break;
            case 6:
                { uint8_t v=rd8(c,b); switch(y){case 0:add_a(c,v,false);break;case 1:add_a(c,v,true);break;case 2:sub_a(c,v,false);break;case 3:sub_a(c,v,true);break;case 4:c->a&=v;setz(c,c->a==0);setn(c,false);seth(c,true);setc(c,false);break;case 5:c->a^=v;setz(c,c->a==0);setn(c,false);seth(c,false);setc(c,false);break;case 6:c->a|=v;setz(c,c->a==0);setn(c,false);seth(c,false);setc(c,false);break;default:cp_a(c,v);break;}cycles=8; }
                break;
            case 7:
                push16(c,b,c->pc); c->pc=(uint16_t)(0x00 + y*8); cycles=16; break;
        }
    }

    c->f &= 0xF0u;
    if (c->ei_delay > 0) {
        c->ei_delay--;
        if (c->ei_delay == 0) c->ime=true;
    }
    return cycles;
}

void cpu_dump(const CPU *c) {
    printf("AF=%02X%02X BC=%02X%02X DE=%02X%02X HL=%02X%02X SP=%04X PC=%04X F=%02X IME=%d HALT=%d\n",
           c->a,c->f,c->b,c->c,c->d,c->e,c->h,c->l,c->sp,c->pc,c->f,c->ime,c->halted);
}
