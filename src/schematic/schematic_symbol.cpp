#include "schematic_symbol.hpp"
#include "pool/part.hpp"
#include "common/lut.hpp"
#include "nlohmann/json.hpp"
#include "schematic.hpp"

namespace horizon {

static const LutEnumStr<SchematicSymbol::PinDisplayMode> pdm_lut = {
        {"selected_only", SchematicSymbol::PinDisplayMode::SELECTED_ONLY},
        {"both", SchematicSymbol::PinDisplayMode::BOTH},
        {"all", SchematicSymbol::PinDisplayMode::ALL},
};

SchematicSymbol::SchematicSymbol(const UUID &uu, const json &j, Pool &pool, Block *block)
    : uuid(uu), pool_symbol(pool.get_symbol(j.at("symbol").get<std::string>())), symbol(*pool_symbol),
      placement(j.at("placement")), smashed(j.value("smashed", false)),
      display_directions(j.value("display_directions", false)), display_all_pads(j.value("display_all_pads", true))
{
    if (j.count("pin_display_mode"))
        pin_display_mode = pdm_lut.lookup(j.at("pin_display_mode"));

    if (block) {
        component = &block->components.at(j.at("component").get<std::string>());
        gate = &component->entity->gates.at(j.at("gate").get<std::string>());
    }
    else {
        component.uuid = j.at("component").get<std::string>();
        gate.uuid = j.at("gate").get<std::string>();
    }
    if (j.count("texts")) {
        const json &o = j.at("texts");
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            texts.emplace_back(UUID(it.value().get<std::string>()));
        }
    }
}
SchematicSymbol::SchematicSymbol(const UUID &uu, const Symbol *sym)
    : uuid(uu), pool_symbol(sym), symbol(*sym), display_all_pads(false)
{
}

json SchematicSymbol::serialize() const
{
    json j;
    j["component"] = (std::string)component.uuid;
    j["gate"] = (std::string)gate.uuid;
    j["symbol"] = (std::string)pool_symbol->uuid;
    j["placement"] = placement.serialize();
    j["smashed"] = smashed;
    j["pin_display_mode"] = pdm_lut.lookup_reverse(pin_display_mode);
    j["display_directions"] = display_directions;
    j["display_all_pads"] = display_all_pads;
    j["texts"] = json::array();
    for (const auto &it : texts) {
        j["texts"].push_back((std::string)it->uuid);
    }

    return j;
}

UUID SchematicSymbol::get_uuid() const
{
    return uuid;
}

std::string SchematicSymbol::replace_text(const std::string &t, bool *replaced, const Schematic &sch) const
{
    if (replaced)
        *replaced = false;
    bool is_value = t == "$VALUE";
    std::string r;
    if (t == "$REFDES" || t == "$RD") {
        if (replaced)
            *replaced = true;
        r = component->refdes + gate->suffix;
    }
    else {
        r = component->replace_text(t, replaced);
    }
    if (is_value && sch.group_tag_visible) {
        if (component->group) {
            r += "\nG:" + sch.block->get_group_name(component->group);
            r += "\nT:" + sch.block->get_tag_name(component->tag);
        }
    }
    return r;
}
} // namespace horizon
