#include <stddef.h>

#include "Settings.h"
#include "Util.h"

using namespace RE::BSScript;
using namespace SKSE;
using namespace SKSE::log;
using namespace SKSE::stl;

namespace {
    void InitializeLogging() {
        auto path = log_directory();
        if (!path) {
            report_and_fail("Unable to lookup SKSE logs directory.");
        }
        *path /= PluginDeclaration::GetSingleton()->GetName();
        *path += L".log";

        std::shared_ptr<spdlog::logger> log;
        if (IsDebuggerPresent()) {
            log = std::make_shared<spdlog::logger>(
                "Global", std::make_shared<spdlog::sinks::msvc_sink_mt>());
        } else {
            log = std::make_shared<spdlog::logger>(
                "Global", std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true));
        }
        log->set_level(spdlog::level::info);
        log->flush_on(spdlog::level::info);

        spdlog::set_default_logger(std::move(log));
        spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] [%t] [%s:%#] %v");
    }

    void MessageHandler(SKSE::MessagingInterface::Message* a_msg) {
        switch (a_msg->type) {
            case SKSE::MessagingInterface::kDataLoaded: {
                RE::TESDataHandler* handler = RE::TESDataHandler::GetSingleton();

                std::vector<RE::TESForm*> blacklist;
                std::string blacklistStr = *Settings::blacklist;
                if (!blacklistStr.empty()) {
                    std::vector<std::string> items = Util::string_split(blacklistStr, ',');
                    for (std::string& item : items) {
                        std::vector<std::string> data = Util::string_split(item, '|');
                        if (data.size() == 2) {
                            RE::TESForm* form = handler->LookupForm(std::stoi(data[1], nullptr, 16), data[0]);
                            if (form) {
                                blacklist.push_back(form);
                            } else {
                                logger::info("unknown form: {}", item);
                            }
                        } else {
                            logger::info("malformed blacklist entry: {}", item);
                        }
                    }
                }

                std::vector<RE::TESForm*> whitelist;
                std::string whitelistStr = *Settings::whitelist;
                if (!whitelistStr.empty()) {
                    std::vector<std::string> items = Util::string_split(whitelistStr, ',');
                    for (std::string& item : items) {
                        std::vector<std::string> data = Util::string_split(item, '|');
                        if (data.size() == 2) {
                            RE::TESForm* form = handler->LookupForm(std::stoi(data[1], nullptr, 16), data[0]);
                            if (form) {
                                whitelist.push_back(form);
                            } else {
                                logger::info("unknown form: {}", item);
                            }
                        } else {
                            logger::info("malformed whitelist entry: {}", item);
                        }
                    }
                }

                if (*Settings::books || !whitelist.empty()) {
                    auto& books = handler->GetFormArray<RE::TESObjectBOOK>();
                    for (RE::TESObjectBOOK* book : books) {
                        if (*Settings::books && !Util::contains(blacklist, book) || Util::contains(whitelist, book)) {
                            book->weight = 0;
                        }
                    }
                }

                if (*Settings::soulgems || !whitelist.empty()) {
                    auto& soulgems = handler->GetFormArray<RE::TESSoulGem>();
                    for (RE::TESSoulGem* soulgem : soulgems) {
                        if (*Settings::soulgems && !Util::contains(blacklist, soulgem) || Util::contains(whitelist, soulgem)) {
                            soulgem->weight = 0;
                        }
                    }
                }

                if (*Settings::ingredients || !whitelist.empty()) {
                    auto& ingredients = handler->GetFormArray<RE::IngredientItem>();
                    for (RE::IngredientItem* ingredient : ingredients) {
                        if (*Settings::ingredients && !Util::contains(blacklist, ingredient) || Util::contains(whitelist, ingredient)) {
                            ingredient->weight = 0;
                        }
                    }
                }

                if (*Settings::food || *Settings::potions || !whitelist.empty()) {
                    RE::BGSKeyword* VendorItemFood = handler->LookupForm<RE::BGSKeyword>(0x08CDEA, "Skyrim.esm");
                    RE::BGSKeyword* VendorItemFoodRaw = handler->LookupForm<RE::BGSKeyword>(0x0A0E56, "Skyrim.esm");

                    auto& potions = handler->GetFormArray<RE::AlchemyItem>();
                    for (RE::AlchemyItem* potion : potions) {
                        if (Util::contains(whitelist, potion)) {
                            potion->weight = 0;
                        } else if (!Util::contains(blacklist, potion)) {
                            if (potion->HasKeyword(VendorItemFood) || potion->HasKeyword(VendorItemFoodRaw)) {
                                if (*Settings::food) {
                                    potion->weight = 0;
                                }
                            } else {
                                if (*Settings::potions) {
                                    potion->weight = 0;
                                }
                            }
                        }
                    }
                }
                
                if (*Settings::scrolls || !whitelist.empty()) {
                    auto& scrolls = handler->GetFormArray<RE::ScrollItem>();
                    for (RE::ScrollItem* scroll : scrolls) {
                        if (*Settings::scrolls && !Util::contains(blacklist, scroll) || Util::contains(whitelist, scroll)) {
                            scroll->weight = 0;
                        }
                    }
                }
                
                if (*Settings::gems || *Settings::ingotsandores || *Settings::animalparts || *Settings::clutter || *Settings::misc || !whitelist.empty()) {
                    RE::BGSKeyword* VendorItemGem = handler->LookupForm<RE::BGSKeyword>(0x0914ED, "Skyrim.esm");
                    RE::BGSKeyword* VendorItemOreIngot = handler->LookupForm<RE::BGSKeyword>(0x0914EC, "Skyrim.esm");
                    RE::BGSKeyword* VendorItemAnimalHide = handler->LookupForm<RE::BGSKeyword>(0x0914EA, "Skyrim.esm");
                    RE::BGSKeyword* VendorItemAnimalPart = handler->LookupForm<RE::BGSKeyword>(0x0914EB, "Skyrim.esm");
                    RE::BGSKeyword* VendorItemClutter = handler->LookupForm<RE::BGSKeyword>(0x0914E9, "Skyrim.esm");

                    auto& misc = handler->GetFormArray<RE::TESObjectMISC>();
                    for (RE::TESObjectMISC* item : misc) {
                        if (Util::contains(whitelist, item)) {
                            item->weight = 0;
                        } else if (!Util::contains(blacklist, item)){
                            if (item->HasKeyword(VendorItemGem)) {
                                if (*Settings::gems) {
                                    item->weight = 0;
                                }
                            } else if (item->HasKeyword(VendorItemOreIngot)) {
                                if (*Settings::ingotsandores) {
                                    item->weight = 0;
                                }
                            } else if (item->HasKeyword(VendorItemAnimalHide) ||
                                       item->HasKeyword(VendorItemAnimalPart)) {
                                if (*Settings::animalparts) {
                                    item->weight = 0;
                                }
                            } else if (item->HasKeyword(VendorItemClutter)) {
                                if (*Settings::clutter) {
                                    item->weight = 0;
                                }
                            } else {
                                if (*Settings::misc) {
                                    item->weight = 0;
                                }
                            }
                        }
                    }
                }

                if (*Settings::light || !whitelist.empty()) {
                    auto& lights = handler->GetFormArray<RE::TESObjectLIGH>();
                    for (RE::TESObjectLIGH* light : lights) {
                        if (*Settings::light && !Util::contains(blacklist, light) || Util::contains(whitelist, light)) {
                            light->weight = 0;
                        }
                    }
                }

                if (*Settings::weapons || !whitelist.empty()) {
                    auto& weapons = handler->GetFormArray<RE::TESObjectWEAP>();
                    for (RE::TESObjectWEAP* weapon : weapons) {
                        if (*Settings::weapons && !Util::contains(blacklist, weapon) || Util::contains(whitelist, weapon)) {
                            weapon->weight = 0;
                        }
                    }
                }
                
                if ((*Settings::ammo || !whitelist.empty()) && REL::Module::GetRuntime() != REL::Module::Runtime::VR) {
                    auto& ammos = handler->GetFormArray<RE::TESAmmo>();
                    for (RE::TESAmmo* ammo : ammos) {
                        if (*Settings::ammo && !Util::contains(blacklist, ammo) || Util::contains(whitelist, ammo)) {
                            reinterpret_cast<RE::TESWeightForm*>(reinterpret_cast<uintptr_t>(ammo) + 0x0B0)->weight = 0;
                        }
                    }
                }
                
                if (*Settings::jewelry || *Settings::armor || !whitelist.empty()) {
                    RE::BGSKeyword* VendorItemJewelry = handler->LookupForm<RE::BGSKeyword>(0x08F95A, "Skyrim.esm");

                    auto& armors = handler->GetFormArray<RE::TESObjectARMO>();
                    for (RE::TESObjectARMO* armor : armors) {
                        if (Util::contains(whitelist, armor)) {
                            armor->weight = 0;
                        } else if (!Util::contains(blacklist, armor)) {
                            if (armor->HasKeyword(VendorItemJewelry)) {
                                if (*Settings::jewelry) {
                                    armor->weight = 0;
                                }
                            } else {
                                if (*Settings::armor) {
                                    armor->weight = 0;
                                }
                            }
                        }
                    }
                }
            } break;
        }
    }
}

SKSEPluginLoad(const LoadInterface* skse) {
    InitializeLogging();

    auto* plugin = PluginDeclaration::GetSingleton();
    auto version = plugin->GetVersion();
    log::info("{} {} is loading...", plugin->GetName(), version);

    Init(skse);

    Settings::load();

    auto message = SKSE::GetMessagingInterface();
    if (!message->RegisterListener(MessageHandler)) {
        return false;
    }

    log::info("{} has finished loading.", plugin->GetName());
    return true;
}
