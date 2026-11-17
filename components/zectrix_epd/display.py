# Zectrix Note 4 e-paper (SSD2683, 400x300) display platform.
# Panel driver ported from the Zectrix ESP-IDF "zectrix_epd" component,
# Copyright (c) 2026 Zectrix Lab, MIT License (see LICENSE-zectrix-epd).

from esphome import pins
import esphome.codegen as cg
from esphome.components import display
import esphome.config_validation as cv
from esphome.const import (
    CONF_BUSY_PIN,
    CONF_CS_PIN,
    CONF_DC_PIN,
    CONF_ID,
    CONF_LAMBDA,
    CONF_PAGES,
    CONF_RESET_PIN,
)

DEPENDENCIES = ["esp32", "psram"]

CONF_POWER_PIN = "power_pin"
CONF_CLK_PIN = "clk_pin"
CONF_MOSI_PIN = "mosi_pin"
CONF_SPI_HOST = "spi_host"
CONF_SPI_FREQUENCY = "spi_frequency"
CONF_BUSY_TIMEOUT = "busy_timeout"
CONF_GRAY_BUSY_TIMEOUT = "gray_busy_timeout"
CONF_FULL_REFRESH_EVERY = "full_refresh_every"
CONF_POWER_OFF_BETWEEN_REFRESHES = "power_off_between_refreshes"

zectrix_epd_ns = cg.esphome_ns.namespace("zectrix_epd")
ZectrixEPD = zectrix_epd_ns.class_(
    "ZectrixEPD", cg.PollingComponent, display.DisplayBuffer
)

SPI_HOSTS = {"SPI2": 2, "SPI3": 3}

CONFIG_SCHEMA = cv.All(
    display.FULL_DISPLAY_SCHEMA.extend(
        {
            cv.GenerateID(): cv.declare_id(ZectrixEPD),
            # Defaults: Zectrix reference zectrix_epd_get_default_config()
            cv.Optional(CONF_CS_PIN, default=11): pins.internal_gpio_output_pin_number,
            cv.Optional(CONF_DC_PIN, default=10): pins.internal_gpio_output_pin_number,
            cv.Optional(
                CONF_RESET_PIN, default=9
            ): pins.internal_gpio_output_pin_number,
            cv.Optional(CONF_BUSY_PIN, default=8): pins.internal_gpio_input_pin_number,
            cv.Optional(
                CONF_POWER_PIN, default=6
            ): pins.internal_gpio_output_pin_number,
            cv.Optional(CONF_CLK_PIN, default=12): pins.internal_gpio_output_pin_number,
            cv.Optional(
                CONF_MOSI_PIN, default=13
            ): pins.internal_gpio_output_pin_number,
            cv.Optional(CONF_SPI_HOST, default="SPI3"): cv.enum(SPI_HOSTS, upper=True),
            cv.Optional(CONF_SPI_FREQUENCY, default="20MHz"): cv.All(
                cv.frequency, cv.Range(min=1e6, max=80e6)
            ),
            cv.Optional(
                CONF_BUSY_TIMEOUT, default="2000ms"
            ): cv.positive_time_period_milliseconds,
            cv.Optional(
                CONF_GRAY_BUSY_TIMEOUT, default="5000ms"
            ): cv.positive_time_period_milliseconds,
            cv.Optional(CONF_FULL_REFRESH_EVERY, default=30): cv.int_range(min=0),
            cv.Optional(CONF_POWER_OFF_BETWEEN_REFRESHES, default=False): cv.boolean,
        }
    ).extend(cv.polling_component_schema("never")),
    cv.only_on_esp32,
    cv.only_with_framework("esp-idf"),
    cv.has_at_most_one_key(CONF_PAGES, CONF_LAMBDA),
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await display.register_display(var, config)

    cg.add(
        var.set_pins(
            config[CONF_CS_PIN],
            config[CONF_DC_PIN],
            config[CONF_RESET_PIN],
            config[CONF_BUSY_PIN],
            config[CONF_POWER_PIN],
            config[CONF_CLK_PIN],
            config[CONF_MOSI_PIN],
        )
    )
    cg.add(var.set_spi_host(config[CONF_SPI_HOST]))
    cg.add(var.set_spi_frequency(int(config[CONF_SPI_FREQUENCY])))
    cg.add(var.set_busy_timeout(config[CONF_BUSY_TIMEOUT].total_milliseconds))
    cg.add(
        var.set_gray_busy_timeout(config[CONF_GRAY_BUSY_TIMEOUT].total_milliseconds)
    )
    cg.add(var.set_full_refresh_every(config[CONF_FULL_REFRESH_EVERY]))
    cg.add(
        var.set_power_off_between_refreshes(config[CONF_POWER_OFF_BETWEEN_REFRESHES])
    )

    if CONF_LAMBDA in config:
        lambda_ = await cg.process_lambda(
            config[CONF_LAMBDA], [(display.DisplayRef, "it")], return_type=cg.void
        )
        cg.add(var.set_writer(lambda_))
