#include <TMC51X0.hpp>


#if defined(ARDUINO_ARCH_RP2040)
SPIClassRP2040 & spi = SPI;
size_t SCK_PIN = 18;
size_t TX_PIN = 19;
size_t RX_PIN = 20;
#else
SPIClass & spi = SPI;
#endif

const tmc51x0::SpiParameters spi_parameters =
{
  .spi_ptr = &spi,
  .chip_select_pin = 8
};

const tmc51x0::ConverterParameters converter_parameters =
{
  // .clock_frequency_mhz = 16, // (typical external clock)
  .microsteps_per_real_position_unit = 4881
};
// clock_frequency_mhz default is 12 (internal clock)
// set clock_frequency_mhz if using external clock instead
// 200 fullsteps per revolution for many steppers * 256 microsteps per fullstep
// 10.49 millimeters per revolution leadscrew -> 51200 / 10.49 ~= 4881
// one "real unit" in this example is one millimeters of linear travel

const tmc51x0::DriverParameters driver_parameters_real =
{
  .run_current = 25, // (percent)
  .motor_direction = tmc51x0::REVERSE,
  .stealth_chop_threshold = 100, // (millimeters/s)
  .stealth_chop_enabled = true,
};

const tmc51x0::ControllerParameters controller_parameters_real =
{
  .ramp_mode = tmc51x0::POSITION,
  .max_velocity = 20, // (millimeters/s)
  .max_acceleration = 2, // ((millimeters/s)/s)
  .start_velocity = 1, // (millimeters/s)
  .stop_velocity = 5, // (millimeters/s)
  .first_velocity = 10, // (millimeters/s)
  .first_acceleration = 10, // ((millimeters/s)/s)
  .max_deceleration = 20, // ((millimeters/s)/s)
  .first_deceleration = 25, // ((millimeters/s)/s)
};

const int32_t MIN_TARGET_POSITION = 20;  // millimeters
const int32_t MAX_TARGET_POSITION = 600;  // millimeters

// const size_t ENABLE_HARDWARE_PIN = 4;

const uint32_t SERIAL_BAUD_RATE = 115200;
const uint16_t LOOP_DELAY = 500;

// global variables
TMC51X0 tmc5130;
uint32_t target_position;

void setup()
{
  Serial.begin(SERIAL_BAUD_RATE);

#if defined(ARDUINO_ARCH_RP2040)
  spi.setSCK(SCK_PIN);
  spi.setTX(TX_PIN);
  spi.setRX(RX_PIN);
#endif
  spi.begin();
  tmc5130.setupSpi(spi_parameters);

  tmc5130.converter.setup(converter_parameters);

  tmc51x0::DriverParameters driver_parameters_chip = tmc5130.converter.driverParametersRealToChip(driver_parameters_real);
  tmc5130.driver.setup(driver_parameters_chip);
  // tmc5130.driver.setEnableHardwarePin(ENABLE_HARDWARE_PIN);

  tmc51x0::ControllerParameters controller_parameters_chip = tmc5130.converter.controllerParametersRealToChip(controller_parameters_real);
  tmc5130.controller.setup(controller_parameters_chip);

  while (!tmc5130.communicating())
  {
    Serial.println("No communication detected, check motor power and connections.");
    delay(LOOP_DELAY);
  }

  while (tmc5130.controller.stepAndDirectionMode())
  {
    Serial.println("Step and Direction mode enabled so SPI/UART motion commands will not work!");
    delay(LOOP_DELAY);
  }

  tmc5130.driver.enable();

  tmc5130.controller.beginRampToZeroVelocity();
  while (!tmc5130.controller.zeroVelocity())
  {
    Serial.println("Waiting for zero velocity.");
    delay(LOOP_DELAY);
  }
  tmc5130.controller.endRampToZeroVelocity();
  tmc5130.controller.zeroActualPosition();

  randomSeed(analogRead(A0));
  long random_delay = random(5000);
  delay(random_delay);

  target_position = MIN_TARGET_POSITION;
  tmc5130.controller.writeTargetPosition(tmc5130.converter.positionRealToChip(target_position));

}

void loop()
{
  tmc5130.printer.readAndPrintRampStat();
  tmc5130.printer.readAndPrintDrvStatus();

  Serial.print("max_velocity (millimeters per second): ");
  Serial.println(controller_parameters_real.max_velocity);

  int32_t actual_velocity_chip = tmc5130.controller.readActualVelocity();
  int32_t actual_velocity_real = tmc5130.converter.velocityChipToReal(actual_velocity_chip);
  Serial.print("actual_velocity (millimeters per second): ");
  Serial.println(actual_velocity_real);

  int32_t actual_position_chip = tmc5130.controller.readActualPosition();
  int32_t actual_position_real = tmc5130.converter.positionChipToReal(actual_position_chip);
  Serial.print("actual position (millimeters): ");
  Serial.println(actual_position_real);

  int32_t target_position_chip = tmc5130.controller.readTargetPosition();
  int32_t target_position_real = tmc5130.converter.positionChipToReal(target_position_chip);
  Serial.print("target position (millimeters): ");
  Serial.println(target_position_real);
  Serial.println("--------------------------");

  if (tmc5130.controller.positionReached())
  {
    Serial.println("Reached target position!");
    Serial.println("--------------------------");
    long random_delay = random(3000);
    delay(random_delay);
    if (target_position == MIN_TARGET_POSITION)
    {
      target_position = MAX_TARGET_POSITION;
    }
    else
    {
      target_position = MIN_TARGET_POSITION;
    }
    tmc5130.controller.writeTargetPosition(tmc5130.converter.positionRealToChip(target_position));
  }

  Serial.println("--------------------------");
  delay(LOOP_DELAY);
}
