#pragma once
#include <cstddef>
namespace kagami {
struct OutputFormat {
    unsigned width=1280,height=720,fps=30;
    bool valid()const {
        return width>=320&&height>=240&&width<=1920&&height<=1920&&width%2==0&&height%2==0&&
            size_t(width)*height<=1920u*1080u&&(fps==15||fps==24||fps==30||fps==60);
    }
};
}
