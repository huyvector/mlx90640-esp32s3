#include "../storage_policy.hpp"
#include <cassert>
#include <cstdio>
int main(){using namespace camera;for(unsigned i=0;i<7;++i){auto s=StorageState(i);assert(!may_format(s,false,false,2,2));assert(!may_format(s,true,true,2,2));assert(!may_format(s,false,true,2,3));assert(may_format(s,false,true,2,2)==(s==StorageState::Ready||s==StorageState::NeedsFormat));assert(should_offer_format(s)==(s==StorageState::NeedsFormat));}puts("PASS: formatting requires consent, idle storage, current card epoch and valid state");}
