#include "Graphic/GraphicSubsystem.h"
#include "Graphic/GraphicSubsystemDX9.h"
#include "Graphic/GraphicSubsystemDX11.h"
#include "Graphic/GraphicSubsystemOpenGL.h"
FGraphicSubsystem* GetGraphicSubsystem(EGraphicSubsystem EGS)
{
    //static FGraphicSubsystemDX9* GraphicSubsystemDX9;
    static FGraphicSubsystemDX11* GraphicSubsystemDX11;
    //static FGraphicSubsystemOPENGL* GraphicSubsystemOPENGL;
    switch (EGS) {
    case  EGraphicSubsystem::DX11: {
        if (!GraphicSubsystemDX11) {
            GraphicSubsystemDX11 = new FGraphicSubsystemDX11;
        }
        return GraphicSubsystemDX11;
    }

    default:
        return nullptr;
    }
}
