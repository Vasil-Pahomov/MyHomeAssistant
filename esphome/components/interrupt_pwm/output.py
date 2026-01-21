import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import output
from esphome.const import CONF_ID, CONF_PIN
from . import interrupt_pwm_ns

DEPENDENCIES = ['esp8266']

InterruptPWMOutput = interrupt_pwm_ns.class_(
    'InterruptPWMOutput', output.FloatOutput, cg.Component
)

CONF_CHANNEL = 'channel'

CONFIG_SCHEMA = output.FLOAT_OUTPUT_SCHEMA.extend({
    cv.Required(CONF_ID): cv.declare_id(InterruptPWMOutput),
    cv.Required(CONF_PIN): cv.int_,
    cv.Required(CONF_CHANNEL): cv.int_range(min=0, max=7),
}).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID], config[CONF_PIN], config[CONF_CHANNEL])
    await cg.register_component(var, config)
    await output.register_output(var, config)
