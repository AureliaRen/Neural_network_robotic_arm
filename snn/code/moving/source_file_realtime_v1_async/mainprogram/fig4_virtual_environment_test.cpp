#include "inc/Fig4VirtualEnvironment.h"
#include <iostream>
#include <stdexcept>
using namespace fig4;
static void check(bool b,const char* m){if(!b)throw std::runtime_error(m);std::cout<<"PASS "<<m<<'\n';}
int main() {
    try {
        VirtualEnvironment e(-4,0,1,.25,12);
        check(e.cue()==RIGHT,"negative position means cue to right");
        for(int k=0;k<3;++k)check(!e.move(TURN_RIGHT).entered,"toward movement before reaching target gives no feedback");
        check(e.position==-1,"position accumulates across actions");
        check(e.move(TURN_RIGHT).entered&&e.position==0,"actual outside-to-inside arrival triggers feedback");
        check(!e.move(NO_DECISION).entered,"remaining in target does not retrigger feedback");
        check(!e.move(TURN_LEFT).entered&&e.position==-1,"leaving target does not trigger feedback");
        check(e.move(TURN_RIGHT).entered,"re-entry triggers a new feedback");
        e.move(TURN_RIGHT);check(e.cue()==LEFT,"crossing target changes observed state");
        VirtualEnvironment boundary(12,0,1,.25,12);
        auto b=boundary.move(TURN_RIGHT);check(b.boundary&&b.after==12&&!b.entered,"boundary clipping is logged without false reward");
        VirtualEnvironment overshoot(-.6,0,1,.25,12);
        check(!overshoot.move(TURN_RIGHT).entered,"endpoint observation does not invent an unobserved crossing reward");
        VirtualEnvironment initial(0,0,1,.25,12);
        check(!initial.move(NO_DECISION).entered,"initially centered position is not a new arrival");
        std::cout<<"VIRTUAL ENVIRONMENT PASS\n";return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
