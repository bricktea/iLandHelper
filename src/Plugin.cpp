#include "Plugin.h"

#include <llapi/EventAPI.h>

using namespace std;
using namespace ll;
Logger logger("iLandHelper");

void PluginMain() {
    logger.info("plugin loaded!");
}

void PluginInit() {
    registerPlugin("iLandHelper", "Helper for iland.",
                   Version{1, 2, 0, Version::Release}, {{"Author", "Redbeanw"}});
    Event::ServerStartedEvent::subscribe([](Event::ServerStartedEvent ev) -> bool {
        PluginMain();
        return true;
    });
}

#include "llapi/RemoteCallAPI.h"
#include "llapi/mc/BlockSource.hpp"

static auto PosGetLand = RemoteCall::importAs<string(unordered_map<string,int>,bool)>("LLSEGlobal","ILAPI_PosGetLand");
static auto GetLandInRange = RemoteCall::importAs<vector<string>(std::unordered_map<string,int>,unordered_map<string,int>,int,bool)>("LLSEGlobal","ILAPI_GetLandInRange");

class ILand {
public:
    static std::optional<string> getPos(BlockSource* region,BlockPos pos);
    static std::optional<vector<string>> getAABB(BlockSource* region, BlockPos min, BlockPos max);

};

std::optional<string> ILand::getPos(BlockSource *region, BlockPos pos) {
    try {
        return PosGetLand(unordered_map<string,int>{
                {"x",pos.x},
                {"y",pos.y},
                {"z",pos.z},
                {"dimid",region->getDimensionId()}
        },false);
    }
    catch (...)
    {
        return std::nullopt;
    }
}

std::optional<vector<string>> ILand::getAABB(BlockSource *region, BlockPos min, BlockPos max) {
    try {
        return GetLandInRange(
                unordered_map<string,int>{
                        {"x",min.x},
                        {"y",min.y},
                        {"z",min.z}},
                unordered_map<string,int>{
                        {"x",max.x},
                        {"y",max.y},
                        {"z",max.z}},
                region->getDimensionId(),
                false);

    }
    catch (...)
    {
        return std::nullopt;
    }
}

bool isInHandleEventFunction = false;
TClasslessInstanceHook(void, "?handleGameEvent@SculkCatalystBlockActor@@UEAAXAEBVGameEvent@@AEBUGameEventContext@@AEAVBlockSource@@@Z",
                       void* a2, void* a3, void* a4)
{
    isInHandleEventFunction = true;
    original(this, a2, a3, a4);
    isInHandleEventFunction = false;
}

TInstanceHook(int, "?getOnDeathExperience@Actor@@QEAAHXZ",
              Actor)
{
    if (!isInHandleEventFunction)
        return original(this);
    auto region = &getRegion();
    auto center = getBlockStandingOn().getPosition();
    auto currentLand = ILand::getPos(region,center);
    if (!currentLand)
    {
        auto nearbyLands = ILand::getAABB(region, center - 8,center + 8);
        if (nearbyLands && !nearbyLands->empty())
            return 0;
    }
    return original(this);
}

TClasslessInstanceHook(char, "?onFertilized@MossBlock@@UEBA_NAEAVBlockSource@@AEBVBlockPos@@PEAVActor@@W4FertilizerType@@@Z",
                       BlockSource* bs, BlockPos bp, Actor* ac, FertilizerType type)
{
    auto currentLand = ILand::getPos(bs,bp);
    if (!currentLand)
    {
        auto nearbyLands = ILand::getAABB(bs, bp - 2,bp + 2);
        if (nearbyLands && !nearbyLands->empty())
            return 0;
    }
    return original(this, bs, bp, ac, type);
}