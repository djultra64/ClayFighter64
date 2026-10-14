// Resize the original game rectangle, retaining its fill, four edges and fade; djultra64.
#include "options_border.h"
#include <array>
#include <stdexcept>
extern "C" void func_8005B1E8(uint8_t*,recomp_context*);
extern "C" void clay_native_options_border(uint8_t*rdram,recomp_context*ctx){
 const uint32_t source=ctx->r4;std::array<int32_t,4> original{},replacement{};
 if(source==0x800DADA0){original={30,67,290,207};replacement={17,67,303,207};}
 else if(source==0x800DADB0){original={17,87,302,149};replacement={17,67,303,169};}
 else {func_8005B1E8(rdram,ctx);return;}
 for(unsigned i=0;i<4;i++)if(MEM_W(i*4,gpr(S32(source)))!=original[i])throw std::runtime_error("Unexpected original Options border rectangle");
 // Rectangle stays ABOVE the separate callee stack, avoiding native saved slots.
 uint32_t rectangle=uint32_t(ctx->r29)-0x80;
 for(unsigned i=0;i<4;i++)MEM_W(i*4,gpr(S32(rectangle)))=replacement[i];
 recomp_context local=*ctx;local.f_odd=&local.f0.u32h;local.r29=ADD32(ctx->r29,-0x100);local.r4=gpr(S32(rectangle));
 const gpr stack=ctx->r29;func_8005B1E8(rdram,&local);local.r29=stack;*ctx=local;ctx->f_odd=&ctx->f0.u32h;
}
