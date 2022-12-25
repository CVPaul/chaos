#include <iostream>

#include "engine.h"
#include "operator.h"
#include "NumCpp.hpp"

int main(){
    /*
    feature::Engine engine = feature::Engine(2);
    constexpr float fPeriod = 5;
    
    op::Open open({ {"fPeriod", fPeriod},{"nWindow", 170} });
    engine.add(0, "open", &open); 

    op::Open lopen({ {"fPeriod", fPeriod},{"nWindow", 170}, {"bLatest", 1} });
    engine.add(0, "lopen", &lopen); 

    op::High high({{"fPeriod", fPeriod},{"nWindow", 150}});
    engine.add(0, "high", &high); 

    op::Low low({{"fPeriod", fPeriod},{"nWindow", 130}});
    engine.add(0, "low", &low); 
    
    op::Close close({{"fPeriod", fPeriod},{"nWindow", 200}});
    engine.add(0, "close", &close); 

    op::Mean ma5({{"nWindow", 5}});
    engine.add(11, "ma5", &ma5, {"close"});

    op::Mean mal5({{"nWindow", 5}});
    engine.add(0, "mal50", &mal5, {"lopen"});

    op::Mean mao5({{"nWindow", 5}});
    engine.add(0, "mao5", &mao5, {"open"});

    op::Min min5({{"nWindow", 5}});
    engine.add(0, "min5", &min5, {"low"});

    op::Max max5({{"nWindow", 5}});
    engine.add(0, "max5", &max5, {"high"});

    int code = engine.compile();
    
    op::Buffer stream(2);
    stream.reset(0);
    for (int i = 0; i < 10000; i++) {
        stream[0] = i;
        stream[1] = i + 3000;
        engine.update(stream);
        std::cout << "stamp=" << i << "|open=" << open.value << "|high=" << high.value
            << "|low=" << low.value << "|close=" << close.value << "|lopen=" << lopen.value
            << "|ma5=" << ma5.value << "|mal5=" << mal5.value
            << "|mao5=" << mao5.value << "|min5=" << min5.value
            << "|max5=" << max5.value << std::endl;
    }
    */
    nc::NdArray<float> a = { {1, 2}, {3, 4}, {5, 6} };
    a.print();
    a[0, 1] = 0.5;
    a[0, 0] = 0.1;
    a.print();
}

