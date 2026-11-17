# Note 4 e-paper UI: screen model, refresh policy and skins.
#
#   note4_ui:
#     id: ui
#     display_id: epd      # optional when there is a single display
#
# Skins are added from YAML (base/skins/<id>.yaml) with
# id(ui).add_skin(new note4_ui::PaperSkin(...)) in an on_boot lambda.
import esphome.codegen as cg
from esphome.components import display
import esphome.config_validation as cv
from esphome.const import CONF_DISPLAY_ID, CONF_ID

CODEOWNERS = []
# note4_ui.h includes font.h, and every skin brings its own font: entries.
DEPENDENCIES = ["display", "font"]

note4_ui_ns = cg.esphome_ns.namespace("note4_ui")
Note4UI = note4_ui_ns.class_("Note4UI", cg.Component)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(Note4UI),
        cv.GenerateID(CONF_DISPLAY_ID): cv.use_id(display.Display),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    disp = await cg.get_variable(config[CONF_DISPLAY_ID])
    cg.add(var.set_display(disp))
