// RT64 wiring adapted from the Tetrisphere and Blast Corps prototypes.
// ClayTality 64 integration: djultra64.
#include "host.h"
#include "graphics_config.h"
#include "input_hud.h"
#include "hle/rt64_application.h"
#include "hle/rt64_workload_queue.h"
#include "ultramodern/ultramodern.hpp"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <cmath>
namespace {
std::array<uint8_t,0x40> rom_header{};
std::array<uint8_t,0x1000> dmem{}, imem{};
std::atomic<uint64_t> dp_syncs{0};
uint32_t mi_intr=0,dpc_start=0,dpc_end=0,dpc_current=0,dpc_status=0,dpc_clock=0,dpc_bufbusy=0,dpc_pipebusy=0,dpc_tmem=0;
void clay_check_interrupts() {if(mi_intr&0x20){++dp_syncs;mi_intr&=~0x20u;}}
class Context final : public ultramodern::renderer::RendererContext {
std::unique_ptr<RT64::Application> app_;
clay::GraphicsOptions options_=clay::graphics_options();
uint64_t next_metrics_vi_=0;
void apply_display_settings(bool notify) {
        app_->userConfig.resolutionMultiplier=options_.resolution_scale();
        app_->userConfig.aspectRatio=options_.widescreen?RT64::UserConfiguration::AspectRatio::Manual:RT64::UserConfiguration::AspectRatio::Original;
        app_->userConfig.aspectTarget=options_.aspect();
        app_->userConfig.refreshRate=options_.refresh?RT64::UserConfiguration::RefreshRate::Manual:RT64::UserConfiguration::RefreshRate::Original;
        app_->userConfig.refreshRateTarget=options_.refresh?options_.refresh:60;
        app_->userConfig.filtering=options_.filter==clay::TextureFilter::Linear?RT64::UserConfiguration::Filtering::Linear:RT64::UserConfiguration::Filtering::Nearest;
        app_->userConfig.threePointFiltering=options_.filter==clay::TextureFilter::Original;
        app_->userConfig.antialiasing=options_.msaa==8?RT64::UserConfiguration::Antialiasing::MSAA8X:options_.msaa==4?RT64::UserConfiguration::Antialiasing::MSAA4X:options_.msaa==2?RT64::UserConfiguration::Antialiasing::MSAA2X:RT64::UserConfiguration::Antialiasing::None;
        if(notify){app_->updateUserConfig(true);next_metrics_vi_=0;fprintf(stderr,"{\"event\":\"display_settings_applied\",\"render_height\":%u,\"widescreen\":%s,\"fps\":%u,\"filter\":%u,\"msaa\":%u}\n",options_.render_height,options_.widescreen?"true":"false",options_.refresh,unsigned(options_.filter),options_.msaa);}
}
public:
Context(uint8_t* rdram,ultramodern::renderer::WindowHandle window) {
        RT64::Application::Core core{};
#ifdef _WIN32
        core.window = window.window;
#else
        core.window = window;
#endif
        core.checkInterrupts = clay_check_interrupts;
        core.HEADER = rom_header.data();
        core.RDRAM = rdram;
        core.DMEM = dmem.data();
        core.IMEM = imem.data();
        core.MI_INTR_REG = &mi_intr;
        core.DPC_START_REG = &dpc_start;
        core.DPC_END_REG = &dpc_end;
        core.DPC_CURRENT_REG = &dpc_current;
        core.DPC_STATUS_REG = &dpc_status;
        core.DPC_CLOCK_REG = &dpc_clock;
        core.DPC_BUFBUSY_REG = &dpc_bufbusy;
        core.DPC_PIPEBUSY_REG = &dpc_pipebusy;
        core.DPC_TMEM_REG = &dpc_tmem;
        const auto* vi = ultramodern::renderer::get_vi_regs();
        core.VI_STATUS_REG = const_cast<std::uint32_t*>(&vi->VI_STATUS_REG);
        core.VI_ORIGIN_REG = const_cast<std::uint32_t*>(&vi->VI_ORIGIN_REG);
        core.VI_WIDTH_REG = const_cast<std::uint32_t*>(&vi->VI_WIDTH_REG);
        core.VI_INTR_REG = const_cast<std::uint32_t*>(&vi->VI_INTR_REG);
        core.VI_V_CURRENT_LINE_REG = const_cast<std::uint32_t*>(&vi->VI_V_CURRENT_LINE_REG);
        core.VI_TIMING_REG = const_cast<std::uint32_t*>(&vi->VI_TIMING_REG);
        core.VI_V_SYNC_REG = const_cast<std::uint32_t*>(&vi->VI_V_SYNC_REG);
        core.VI_H_SYNC_REG = const_cast<std::uint32_t*>(&vi->VI_H_SYNC_REG);
        core.VI_LEAP_REG = const_cast<std::uint32_t*>(&vi->VI_LEAP_REG);
        core.VI_H_START_REG = const_cast<std::uint32_t*>(&vi->VI_H_START_REG);
        core.VI_V_START_REG = const_cast<std::uint32_t*>(&vi->VI_V_START_REG);
        core.VI_V_BURST_REG = const_cast<std::uint32_t*>(&vi->VI_V_BURST_REG);
        core.VI_X_SCALE_REG = const_cast<std::uint32_t*>(&vi->VI_X_SCALE_REG);
        core.VI_Y_SCALE_REG = const_cast<std::uint32_t*>(&vi->VI_Y_SCALE_REG);

        RT64::ApplicationConfiguration config{};
        config.appId = "claytality64";
        config.useConfigurationFile = false;
        app_ = std::make_unique<RT64::Application>(core, config);
        app_->userConfig.graphicsAPI = RT64::UserConfiguration::GraphicsAPI::Vulkan;
        app_->userConfig.resolution = RT64::UserConfiguration::Resolution::Manual;
        app_->userConfig.resolutionMultiplier = options_.resolution_scale();
        app_->userConfig.aspectRatio = options_.widescreen ? RT64::UserConfiguration::AspectRatio::Manual : RT64::UserConfiguration::AspectRatio::Original;
        app_->userConfig.aspectTarget = options_.aspect();
        // Match/interpolate submitted workloads; guest VI/simulation timing is untouched.
        app_->userConfig.refreshRate = options_.refresh ? RT64::UserConfiguration::RefreshRate::Manual : RT64::UserConfiguration::RefreshRate::Original;
        app_->userConfig.refreshRateTarget = options_.refresh ? options_.refresh : 60;
        app_->enhancementConfig.presentation.mode = RT64::EnhancementConfiguration::Presentation::Mode::SkipBuffering;
        apply_display_settings(false);
        clay::input::install_hud();
        auto result=app_->setup(0);
        setup_result=static_cast<ultramodern::renderer::SetupResult>(result);
        chosen_api=ultramodern::renderer::GraphicsApi::Vulkan;
        if (result!=RT64::Application::SetupResult::Success) clay_fatal("rt64_setup",static_cast<uint32_t>(result));
        fprintf(stderr,"{\"event\":\"rt64_device_ready\",\"gpu\":\"%s\"}\n",app_->device->getDescription().name.c_str());
        fprintf(stderr,"{\"event\":\"graphics_requested\",\"output_width\":%u,\"output_height\":%u,\"render_height\":%u,\"scale\":%.4f,\"aspect\":%.6f,\"widescreen\":%s,\"presentation_rate\":%u}\n",options_.width,options_.height,options_.render_height,options_.resolution_scale(),options_.aspect(),options_.widescreen?"true":"false",options_.refresh);
        fflush(stderr);
    }
    bool valid() override { return bool(app_); }
    bool update_config(const ultramodern::renderer::GraphicsConfig&,const ultramodern::renderer::GraphicsConfig&) override { return true; }
    void enable_instant_present() override {}
    void send_dl(const OSTask* task) override {
        auto selected=clay::graphics_options();if(selected!=options_){options_=selected;apply_display_settings(true);}
        const auto sync_before=dp_syncs.load();
        ++clay_gfx;
        app_->state->rsp->reset();
        app_->interpreter->loadUCodeGBI(task->t.ucode & 0x3FFFFFF, task->t.ucode_data & 0x3FFFFFF, true);
        if (!app_->interpreter->hleGBI || app_->interpreter->hleGBI->ucode == RT64::GBIUCode::Unknown) clay_fatal("unsupported_graphics_ucode", task->t.ucode);
        fprintf(stderr, "{\"event\":\"gfx_task\",\"count\":%llu,\"ucode\":%u}\n", (unsigned long long)clay_gfx.load(), (unsigned)app_->interpreter->hleGBI->ucode);
        // RT64 AUTO otherwise stretches rectangles spanning the full framebuffer.
        // Preserve sprite/menu/HUD image proportions while 3D projections expand.
        app_->state->rdp->setRectAspect(options_.widescreen ? G_EX_ASPECT_ADJUST : G_EX_ASPECT_AUTO);
        app_->processDisplayLists(app_->core.RDRAM, task->t.data_ptr & 0x3FFFFFF, 0, true);
        if (std::getenv("CLAY_TRACE_COMPLETION")) fprintf(stderr,"{\"event\":\"gfx_consumed\",\"gfx\":%llu,\"fullsyncs\":%llu,\"list\":\"%08X\"}\n",(unsigned long long)clay_gfx.load(),(unsigned long long)(dp_syncs.load()-sync_before),task->t.data_ptr);
    }
    void send_dummy_workload(uint32_t framebuffer) override {
        app_->state->listProcessBegin();
        app_->state->rdp->setColorImage(G_IM_FMT_RGBA,G_IM_SIZ_16b,320,framebuffer);
        app_->state->rdp->setOtherMode(0x382C30,0);
        app_->state->rdp->fillRect(0,0,320<<2,240<<2);
        app_->state->fullSync();
        app_->state->listProcessEnd();
    }
    void update_screen() override {
        app_->updateScreen();
        const auto vi=clay_vis.load();if(vi<next_metrics_vi_)return;next_metrics_vi_=vi+300;
        auto& shared=*app_->sharedQueueResources;
        uint32_t target=0,original=0,display=0,output_width=0,output_height=0,render_width=0,render_height=0,batch_count=0,batch_presented=0;
        {std::lock_guard lock(shared.configurationMutex);target=shared.targetRate;original=shared.viOriginalRate;display=shared.swapChainRate;output_width=shared.swapChainWidth;output_height=shared.swapChainHeight;}
        {std::lock_guard lock(shared.managerMutex);for(auto& [key,fb]:shared.renderTargetManager.targetMap)if(uint64_t(fb->width)*fb->height>uint64_t(render_width)*render_height){render_width=fb->width;render_height=fb->height;}}
        {std::lock_guard lock(shared.interpolatedMutex);for(const auto& batch:shared.interpolatedFrames){batch_count+=batch.count;batch_presented+=batch.presented;}}
        // Batch counters describe the recent interpolation batches, not measured FPS.
        fprintf(stderr,"{\"event\":\"graphics_effective\",\"vi\":%llu,\"render_width\":%u,\"render_height\":%u,\"output_width\":%u,\"output_height\":%u,\"requested_rate\":%u,\"effective_target_rate\":%u,\"original_rate\":%u,\"display_rate\":%u,\"batch_frames\":%u,\"batch_presented\":%u}\n",(unsigned long long)vi,render_width,render_height,output_width,output_height,options_.refresh,target,original,display,batch_count,batch_presented);
    }
    void shutdown() override { app_->end(); }
    uint32_t get_display_framerate() const override { return app_->presentQueue->ext.sharedResources->swapChainRate; }
    float get_resolution_scale() const override { return float(options_.resolution_scale()); }
};
}
std::unique_ptr<ultramodern::renderer::RendererContext> clay_create_context(uint8_t* rdram,ultramodern::renderer::WindowHandle window,bool) { return std::make_unique<Context>(rdram,window); }
