#include "../Commands/SDCARD.h"

#include "../../ESPEasy_common.h"
#include "../Commands/Common.h"


#if FEATURE_SD

#include "../ESPEasyCore/Serial.h"
#include "../Globals/Settings.h"
#include "../Helpers/StringConverter.h"

#include <SD.h>


void printDirectory(fs::File dir, int numTabs)
{
  while (true) {
    fs::File entry = dir.openNextFile();

    if (!entry) {
      // no more files
      break;
    }

    for (uint8_t i = 0; i < numTabs; i++) {
      serialPrint("\t");
    }
    serialPrint(entry.name());

    if (entry.isDirectory()) {
      serialPrintln("/");
      printDirectory(entry, numTabs + 1);
    } else {
      // files have sizes, directories do not
      serialPrint("\t\t");
      serialPrintln(String(entry.size(), DEC));
    }
    entry.close();
  }
}


const __FlashStringHelper * Command_SD_LS(struct EventStruct *event, const char* Line)
{
  fs::File root = SD.open("/");
  root.rewindDirectory();
  printDirectory(root, 0);
  root.close();
  return return_see_serial(event);
}

String Command_SD_Remove(struct EventStruct *event, const char* Line)
{
  // FIXME TD-er: This one is not using parseString* function
  String fname = Line;
  fname = fname.substring(9);
  SD.remove((char*)fname.c_str());
  return return_result(event, concat(F("Removing:"), fname));
}
#endif

#include "../Commands/Common.h"

#include <ctype.h>
#include <IPAddress.h>

#include "../../ESPEasy_common.h"

#include "../DataStructs/ESPEasy_EventStruct.h"
#include "../DataTypes/EventValueSource.h"

#include "../ESPEasyCore/ESPEasyWifi.h"
#include "../ESPEasyCore/Serial.h"

#include "../Helpers/Networking.h"
#include "../Helpers/Numerical.h"
#include "../Helpers/StringConverter.h"


// Simple function to return "Ok", to avoid flash string duplication in the firmware.
const __FlashStringHelper * return_command_success()
{
  return F("\nOK");
}

const __FlashStringHelper * return_command_failed()
{
  return F("\nFailed");
}

String return_command_success_str()
{
  return return_command_success();
}

String return_command_failed_str()
{
  return return_command_failed();
}

const __FlashStringHelper * return_incorrect_nr_arguments()
{
  return F("Too many arguments, try using quotes!");
}

const __FlashStringHelper * return_incorrect_source()
{
  return F("Command not allowed from this source!");
}

const __FlashStringHelper * return_not_connected()
{
  return F("Not connected to WiFi");
}

String return_result(struct EventStruct *event, const String& result)
{
  serialPrintln();
  serialPrintln(result);

  if (event->Source == EventValueSource::Enum::VALUE_SOURCE_SERIAL) {
    return return_command_success_str();
  }
  return result;
}

const __FlashStringHelper * return_see_serial(struct EventStruct *event)
{
  if (event->Source == EventValueSource::Enum::VALUE_SOURCE_SERIAL) {
    return return_command_success();
  }
  return F("Output sent to serial");
}

String Command_GetORSetIP(struct EventStruct *event,
                          const __FlashStringHelper * targetDescription,
                          const char         *Line,
                          uint8_t               *IP,
                          const IPAddress   & dhcpIP,
                          int                 arg)
{
  bool hasArgument = false;
  {
    // Check if command is valid. Leave in separate scope to delete the TmpStr1
    String TmpStr1;

    if (GetArgv(Line, TmpStr1, arg + 1)) {
      hasArgument = true;

      if (!str2ip(TmpStr1, IP)) {
        return return_result(event, concat(F("Invalid parameter: "), TmpStr1));
      }
    }
  }

  if (!hasArgument) {
    String result = targetDescription;

    if (useStaticIP()) {
      result += formatIP(IP);
    } else {
      result += formatIP(dhcpIP);
      result += F("(DHCP)");
    }
    return return_result(event, result);
  }
  return return_command_success_str();
}

String Command_GetORSetString(struct EventStruct *event,
                              const __FlashStringHelper * targetDescription,
                              const char         *Line,
                              char               *target,
                              size_t              len,
                              int                 arg
                              )
{
  bool hasArgument = false;
  {
    // Check if command is valid. Leave in separate scope to delete the TmpStr1
    String TmpStr1;

    if (GetArgv(Line, TmpStr1, arg + 1)) {
      hasArgument = true;

      if (TmpStr1.length() > len) {
        String result = concat(targetDescription, F(" is too large. max size is "));
        result += len;
        return return_result(event, result);
      }
      safe_strncpy(target, TmpStr1, len);
    }
  }

  if (hasArgument) {
    String result = targetDescription;
    result += target;
    return return_result(event, result);
  }
  return return_command_success_str();
}

String Command_GetORSetBool(struct EventStruct *event,
                            const __FlashStringHelper * targetDescription,
                            const char         *Line,
                            bool               *value,
                            int                 arg)
{
  bool hasArgument = false;
  {
    // Check if command is valid. Leave in separate scope to delete the TmpStr1
    String TmpStr1;

    if (GetArgv(Line, TmpStr1, arg + 1)) {
      hasArgument = true;
      TmpStr1.toLowerCase();

      int tmp_int = 0;
      if (validIntFromString(TmpStr1, tmp_int)) {
        *value = tmp_int > 0;
      }
      else if (strcmp_P(PSTR("on"), TmpStr1.c_str()) == 0) { *value = true; }
      else if (strcmp_P(PSTR("true"), TmpStr1.c_str()) == 0) { *value = true; }
      else if (strcmp_P(PSTR("off"), TmpStr1.c_str()) == 0) { *value = false; }
      else if (strcmp_P(PSTR("false"), TmpStr1.c_str()) == 0) { *value = false; }
    }
  }

  if (hasArgument) {
    return return_result(event, concat(targetDescription, boolToString(*value)));
  }
  return return_command_success_str();
}

#if FEATURE_ETHERNET
String Command_GetORSetETH(struct EventStruct *event,
                            const __FlashStringHelper * targetDescription,
                            const __FlashStringHelper * valueToString,
                            const char         *Line,
                            uint8_t            *value,
                            int                 arg)
{
  bool hasArgument = false;
  {
    // Check if command is valid. Leave in separate scope to delete the TmpStr1
    String TmpStr1;

    if (GetArgv(Line, TmpStr1, arg + 1)) {
      hasArgument = true;
      TmpStr1.toLowerCase();

      int tmp_int = 0;
      if (validIntFromString(TmpStr1, tmp_int)) {
        *value = static_cast<uint8_t>(tmp_int);
      }

      // FIXME TD-er: This should not be in a generic function, but rather pre-processed in the command itself


      // WiFi/Eth mode
      else if (TmpStr1.equals(F("wifi"))) { *value = 0; }
      else if (TmpStr1.equals(F("ethernet"))) { *value = 1; }

      // ETH clockMode
      else if (TmpStr1.startsWith(F("ext"))) { *value = 0; }
      else if (TmpStr1.indexOf(F("gpio0"))  != -1) { *value = 1; }
      else if (TmpStr1.indexOf(F("gpio16")) != -1) { *value = 2; }
      else if (TmpStr1.indexOf(F("gpio17")) != -1) { *value = 3; }
    }
  }

  String result = targetDescription;
  if (hasArgument) {
    result += *value;
  } else {
    result += valueToString;
  }
  return return_result(event, result);
}
#endif

String Command_GetORSetInt8_t(struct EventStruct *event,
                            const __FlashStringHelper * targetDescription,
                            const char         *Line,
                            int8_t             *value,
                            int                 arg)
{
  bool hasArgument = false;
  {
    // Check if command is valid. Leave in separate scope to delete the TmpStr1
    String TmpStr1;

    if (GetArgv(Line, TmpStr1, arg + 1)) {
      hasArgument = true;
      TmpStr1.toLowerCase();

      int tmp_int = 0;
      if (validIntFromString(TmpStr1, tmp_int)) {
        *value = static_cast<int8_t>(tmp_int);
      }
    }
  }

  if (hasArgument) {
    String result = targetDescription;
    result += *value;
    return return_result(event, result);
  }
  return return_command_success_str();
}

#include "../../ESPEasy_common.h"
#include "../Globals/MQTT.h"

#if FEATURE_MQTT



#include "../Commands/Common.h"
#include "../Commands/MQTT.h"

#include "../ESPEasyCore/Controller.h"
#include "../ESPEasyCore/ESPEasy_Log.h"

#include "../Globals/CPlugins.h"
#include "../Globals/ESPEasy_Scheduler.h"
#include "../Globals/Settings.h"

#include "../Helpers/ESPEasy_Storage.h"
#include "../Helpers/PeriodicalActions.h"
#include "../Helpers/Scheduler.h"
#include "../Helpers/StringConverter.h"


const __FlashStringHelper * Command_MQTT_Publish(struct EventStruct *event, const char *Line)
{
  // ToDo TD-er: Not sure about this function, but at least it sends to an existing MQTTclient
  controllerIndex_t enabledMqttController = firstEnabledMQTT_ControllerIndex();

  if (!validControllerIndex(enabledMqttController)) {
    return F("No MQTT controller enabled");
  }

  // Command structure:  Publish,<topic>,<value>
  const String topic = parseStringKeepCase(Line, 2);
  const String value = tolerantParseStringKeepCase(Line, 3);
  # ifndef BUILD_NO_DEBUG
  addLog(LOG_LEVEL_DEBUG, String(F("Publish: ")) + topic + value);
  #endif

  if ((topic.length() > 0) && (value.length() > 0)) {

    bool mqtt_retainFlag;
    {
      // Place the ControllerSettings in a scope to free the memory as soon as we got all relevant information.
      MakeControllerSettings(ControllerSettings); //-V522
      if (!AllocatedControllerSettings()) {
        addLog(LOG_LEVEL_ERROR, F("MQTT : Cannot publish, out of RAM"));
        return F("MQTT : Cannot publish, out of RAM");
      }

      LoadControllerSettings(enabledMqttController, ControllerSettings);
      mqtt_retainFlag = ControllerSettings.mqtt_retainFlag();
    }


    // @giig1967g: if payload starts with '=' then treat it as a Formula and evaluate accordingly
    // The evaluated value is already present in event->Par2
    // FIXME TD-er: Is the evaluated value always present in event->Par2 ?
    // Should it already be evaluated, or should we evaluate it now?

    bool success = false;
    if (value[0] != '=') {
      success = MQTTpublish(enabledMqttController, INVALID_TASK_INDEX, topic.c_str(), value.c_str(), mqtt_retainFlag);
    }
    else {
      success = MQTTpublish(enabledMqttController, INVALID_TASK_INDEX,  topic.c_str(), String(event->Par2).c_str(), mqtt_retainFlag);
    }
    if (success) {
      return return_command_success();
    }
  }
  return return_command_failed();
}


boolean MQTTsubscribe(controllerIndex_t controller_idx, const char* topic, boolean retained)
{
  if (MQTTclient.subscribe(topic)) {
    Scheduler.setIntervalTimerOverride(ESPEasy_Scheduler::IntervalTimer_e::TIMER_MQTT, 10); // Make sure the MQTT is being processed as soon as possible.
    scheduleNextMQTTdelayQueue();
    if (loglevelActiveFor(LOG_LEVEL_INFO)) {
      String log = F("Subscribed to: ");  
      log += topic;
      addLogMove(LOG_LEVEL_INFO, log);
    }
    return true;
  }
  addLog(LOG_LEVEL_ERROR, F("MQTT : subscribe failed"));
  return false;
}

const __FlashStringHelper * Command_MQTT_Subscribe(struct EventStruct *event, const char* Line)
{
  if (MQTTclient.connected() ) {
    // ToDo TD-er: Not sure about this function, but at least it sends to an existing MQTTclient
    controllerIndex_t enabledMqttController = firstEnabledMQTT_ControllerIndex();
    if (validControllerIndex(enabledMqttController)) {
      bool mqtt_retainFlag;
      {
        // Place the ControllerSettings in a scope to free the memory as soon as we got all relevant information.
        MakeControllerSettings(ControllerSettings); //-V522
        if (!AllocatedControllerSettings()) {
          addLog(LOG_LEVEL_ERROR, F("MQTT : Cannot subscribe, out of RAM"));
          return F("MQTT : Cannot subscribe, out of RAM");
        }
        LoadControllerSettings(event->ControllerIndex, ControllerSettings);
        mqtt_retainFlag = ControllerSettings.mqtt_retainFlag();
      }

      String eventName = Line;
      String topic = eventName.substring(10);
      if (!MQTTsubscribe(enabledMqttController, topic.c_str(), mqtt_retainFlag))
         return return_command_failed();
      return return_command_success();
    }
    return F("No MQTT controller enabled");
  }
  return return_not_connected();
}


#endif // if FEATURE_MQTT

#include "../Commands/i2c.h"

#include "../Commands/Common.h"
#include "../ESPEasyCore/Serial.h"

#include "../Globals/I2Cdev.h"

#include "../../ESPEasy_common.h"

const __FlashStringHelper * Command_i2c_Scanner(struct EventStruct *event, const char* Line)
{
	uint8_t error, address;
	for (address = 1; address <= 127; address++) {
		Wire.beginTransmission(address);
		error = Wire.endTransmission();
		if (error == 0) {
			serialPrint(F("I2C  : Found 0x"));
			serialPrintln(String(address, HEX));
		}else if (error == 4) {
			serialPrint(F("I2C  : Error at 0x"));
			serialPrintln(String(address, HEX));
		}
	}
	return return_see_serial(event);
}

#include "../Commands/InternalCommands.h"

#include "../../ESPEasy_common.h"

#include "../../_Plugin_Helper.h"
#include "../Globals/Settings.h"

#if FEATURE_BLYNK
# include "../Commands/Blynk.h"
# include "../Commands/Blynk_c015.h"
#endif // if FEATURE_BLYNK

#include "../Commands/Common.h"
#include "../Commands/Controller.h"
#include "../Commands/Diagnostic.h"
#include "../Commands/GPIO.h"
#include "../Commands/HTTP.h"
#include "../Commands/i2c.h"

#if FEATURE_MQTT
# include "../Commands/MQTT.h"
#endif // if FEATURE_MQTT

#include "../Commands/Networks.h"
#if FEATURE_NOTIFIER
#include "../Commands/Notifications.h"
#endif
#include "../Commands/Provisioning.h"
#include "../Commands/RTC.h"
#include "../Commands/Rules.h"
#include "../Commands/SDCARD.h"
#include "../Commands/Settings.h"
#include "../Commands/Servo.h"
#include "../Commands/System.h"
#include "../Commands/Tasks.h"
#include "../Commands/Time.h"
#include "../Commands/Timer.h"
#include "../Commands/UPD.h"
#include "../Commands/wd.h"
#include "../Commands/WiFi.h"

#include "../ESPEasyCore/ESPEasy_Log.h"

#include "../Helpers/Misc.h"
#include "../Helpers/StringConverter.h"
#include "../Helpers/StringParser.h"


bool checkNrArguments(const char *cmd, const String& Line, int nrArguments) {
  if (nrArguments < 0) { return true; }

  // 0 arguments means argument on pos1 is valid (the command) and argpos 2 should not be there.
  if (HasArgv(Line.c_str(), nrArguments + 2)) {
    #ifndef BUILD_NO_DEBUG
    if (loglevelActiveFor(LOG_LEVEL_ERROR)) {
      String log;
      if (log.reserve(128)) {
        log += F("Too many arguments: cmd=");
        log += cmd;

        if (nrArguments < 1) {
          log += Line;
        } else {
          // Check for one more argument than allowed, since we apparently have one.
          bool done = false;
          int  i    = 1;

          while (!done) {
            String parameter;

            if (i == nrArguments) {
              parameter = tolerantParseStringKeepCase(Line, i + 1);
            } else {
              parameter = parseStringKeepCase(Line, i + 1);
            }
            done = parameter.isEmpty();

            if (!done) {
              if (i <= nrArguments) {
                if (Settings.TolerantLastArgParse() && (i == nrArguments)) {
                  log += F(" (fixed)");
                }
                log += F(" Arg");
              } else {
                log += F(" ExtraArg");
              }
              log += String(i);
              log += '=';
              log += parameter;
            }
            ++i;
          }
        }
        log += F(" lineLength=");
        log += Line.length();
        addLogMove(LOG_LEVEL_ERROR, log);
        log  = F("Line: _");
        log += Line;
        log += '_';
        addLogMove(LOG_LEVEL_ERROR, log);

        if (!Settings.TolerantLastArgParse()) {
          log = F("Command not executed!");
        } else {
          log = F("Command executed, but may fail.");
        }
        log += F(" See: https://github.com/letscontrolit/ESPEasy/issues/2724");
        addLogMove(LOG_LEVEL_ERROR, log);
      }
    }
    #endif

    if (Settings.TolerantLastArgParse()) {
      return true;
    }
    return false;
  }
  return true;
}

bool checkSourceFlags(EventValueSource::Enum source, EventValueSourceGroup::Enum group) {
  if (EventValueSource::partOfGroup(source, group)) {
    return true;
  }
  addLog(LOG_LEVEL_ERROR, return_incorrect_source());
  return false;
}

command_case_data::command_case_data(const char *cmd, struct EventStruct *event, const char *line) :
  cmd(cmd), event(event), line(line)
{
  cmd_lc = cmd;
  cmd_lc.toLowerCase();
}


// Wrapper to reduce generated code by macro
bool do_command_case_all(command_case_data         & data,
                         const __FlashStringHelper * cmd_test,
                         command_function_fs         pFunc,
                         int                         nrArguments)
{
  return do_command_case(data, cmd_test, pFunc, nrArguments, EventValueSourceGroup::Enum::ALL);
}


bool do_command_case_all(command_case_data         & data,
                         const __FlashStringHelper * cmd_test,
                         command_function            pFunc,
                         int                         nrArguments)
{
  return do_command_case(data, cmd_test, pFunc, nrArguments, EventValueSourceGroup::Enum::ALL);
}

// Wrapper to reduce generated code by macro
bool do_command_case_all_restricted(command_case_data         & data,
                                    const __FlashStringHelper * cmd_test,
                                    command_function_fs         pFunc,
                                    int                         nrArguments)
{
  return do_command_case(data, cmd_test, pFunc, nrArguments, EventValueSourceGroup::Enum::RESTRICTED);
}


bool do_command_case_all_restricted(command_case_data         & data,
                                    const __FlashStringHelper * cmd_test,
                                    command_function            pFunc,
                                    int                         nrArguments)
{
  return do_command_case(data, cmd_test, pFunc, nrArguments, EventValueSourceGroup::Enum::RESTRICTED);
}


bool do_command_case_check(command_case_data         & data,
                           const __FlashStringHelper * cmd_test,
                           int                         nrArguments,
                           EventValueSourceGroup::Enum group)
{
  // The data struct is re-used on each attempt to process an internal command.
  // Re-initialize the only two members that may have been altered by a previous call.
  data.retval = false;
  data.status = String();
  if (!data.cmd_lc.equals(cmd_test)) {
    return false;
  }
  if (!checkSourceFlags(data.event->Source, group)) {
    data.status = return_incorrect_source();
    return false;
  } 
  // FIXME TD-er: Do not check nr arguments from MQTT source.
  // See https://github.com/letscontrolit/ESPEasy/issues/3344
  // C005 does recreate command partly from topic and published message
  // e.g. ESP_Easy/Bathroom_pir_env/GPIO/14 with data 0 or 1
  // This only allows for 2 parameters, but some commands need more arguments (default to "0")
  const bool mustCheckNrArguments = data.event->Source != EventValueSource::Enum::VALUE_SOURCE_MQTT;
  if (mustCheckNrArguments) {
    if (!checkNrArguments(data.cmd, data.line, nrArguments)) {
      data.status = return_incorrect_nr_arguments();
      //data.retval = false;
      return true; // Command is handled
    }
  }
  data.retval = true; // Mark the command should be executed.
  return true; // Command is handled
}

bool do_command_case(command_case_data         & data,
                     const __FlashStringHelper * cmd_test,
                     command_function_fs         pFunc,
                     int                         nrArguments,
                     EventValueSourceGroup::Enum group)
{
  if (do_command_case_check(data, cmd_test, nrArguments, group)) {
    // It has been handled, check if we need to execute it.
    // FIXME TD-er: Must change command function signature to use const String&
    data.status = pFunc(data.event, data.line.c_str());
    return true;
  }
  return false;
}


bool do_command_case(command_case_data         & data,
                     const __FlashStringHelper * cmd_test,
                     command_function            pFunc,
                     int                         nrArguments,
                     EventValueSourceGroup::Enum group)
{
  if (do_command_case_check(data, cmd_test, nrArguments, group)) {
    // It has been handled, check if we need to execute it.
    // FIXME TD-er: Must change command function signature to use const String&
    data.status = pFunc(data.event, data.line.c_str());
    return true;
  }
  return false;
}

bool executeInternalCommand(command_case_data & data)
{
  const size_t cmd_lc_length = data.cmd_lc.length();
  if (cmd_lc_length < 2) return false; // No commands less than 2 characters
  // Simple macro to match command to function call.

  // EventValueSourceGroup::Enum::ALL
  #define COMMAND_CASE_A(S, C, NARGS) \
  if (do_command_case_all(data, F(S), &C, NARGS)) { return data.retval; }

  // EventValueSourceGroup::Enum::RESTRICTED
  #define COMMAND_CASE_R(S, C, NARGS) \
  if (do_command_case_all_restricted(data, F(S), &C, NARGS)) { return data.retval; }

  // FIXME TD-er: Should we execute command when number of arguments is wrong?

  // FIXME TD-er: must determine nr arguments where NARGS is set to -1
  switch (data.cmd_lc[0]) {
    case 'a': {
      COMMAND_CASE_A("accessinfo", Command_AccessInfo_Ls,       0); // Network Command
      COMMAND_CASE_A("asyncevent", Command_Rules_Async_Events, -1); // Rule.h
      break;
    }
    case 'b': {
    #ifndef BUILD_NO_DIAGNOSTIC_COMMANDS
      COMMAND_CASE_R("background", Command_Background, 1); // Diagnostic.h
    #endif // ifndef BUILD_NO_DIAGNOSTIC_COMMANDS
    #ifdef USES_C012
      COMMAND_CASE_A("blynkget", Command_Blynk_Get, -1);
    #endif // ifdef USES_C012
    #ifdef USES_C015
      COMMAND_CASE_R("blynkset", Command_Blynk_Set, -1);
    #endif // ifdef USES_C015
      COMMAND_CASE_A("build", Command_Settings_Build, 1);      // Settings.h
      break;
    }
    case 'c': {
      COMMAND_CASE_R( "clearaccessblock", Command_AccessInfo_Clear,   0); // Network Command
      COMMAND_CASE_R(    "clearpassword", Command_Settings_Password_Clear,     1); // Settings.h
      COMMAND_CASE_R(      "clearrtcram", Command_RTC_Clear,          0); // RTC.h
      COMMAND_CASE_R(           "config", Command_Task_RemoteConfig, -1); // Tasks.h
      COMMAND_CASE_R("controllerdisable", Command_Controller_Disable, 1); // Controller.h
      COMMAND_CASE_R( "controllerenable", Command_Controller_Enable,  1); // Controller.h

      break;
    }
    case 'd': {
      COMMAND_CASE_R( "datetime", Command_DateTime,         2); // Time.h
      COMMAND_CASE_R(    "debug", Command_Debug,            1); // Diagnostic.h
      COMMAND_CASE_R("deepsleep", Command_System_deepSleep, 1); // System.h
      COMMAND_CASE_R(    "delay", Command_Delay,            1); // Timers.h
      COMMAND_CASE_R(      "dns", Command_DNS,              1); // Network Command
      COMMAND_CASE_R(      "dst", Command_DST,              1); // Time.h
      break;
    }
    case 'e': {
    #if FEATURE_ETHERNET
      COMMAND_CASE_R(   "ethphyadr", Command_ETH_Phy_Addr,   1); // Network Command
      COMMAND_CASE_R(   "ethpinmdc", Command_ETH_Pin_mdc,    1); // Network Command
      COMMAND_CASE_R(  "ethpinmdio", Command_ETH_Pin_mdio,   1); // Network Command
      COMMAND_CASE_R( "ethpinpower", Command_ETH_Pin_power,  1); // Network Command
      COMMAND_CASE_R(  "ethphytype", Command_ETH_Phy_Type,   1); // Network Command
      COMMAND_CASE_R("ethclockmode", Command_ETH_Clock_Mode, 1); // Network Command
      COMMAND_CASE_R(       "ethip", Command_ETH_IP,         1); // Network Command
      COMMAND_CASE_R(  "ethgateway", Command_ETH_Gateway,    1); // Network Command
      COMMAND_CASE_R(   "ethsubnet", Command_ETH_Subnet,     1); // Network Command  
      COMMAND_CASE_R(      "ethdns", Command_ETH_DNS,        1); // Network Command
      COMMAND_CASE_A("ethdisconnect", Command_ETH_Disconnect, 0); // Network Command
      COMMAND_CASE_R( "ethwifimode", Command_ETH_Wifi_Mode,  1); // Network Command
    #endif // FEATURE_ETHERNET
      COMMAND_CASE_R("erasesdkwifi", Command_WiFi_Erase,     0); // WiFi.h
      COMMAND_CASE_A(       "event", Command_Rules_Events,  -1); // Rule.h
      COMMAND_CASE_A("executerules", Command_Rules_Execute, -1); // Rule.h
      break;
    }
    case 'g': {
      COMMAND_CASE_R(   "gateway", Command_Gateway,     1); // Network Command
      COMMAND_CASE_A(      "gpio", Command_GPIO,        2); // Gpio.h
      COMMAND_CASE_A("gpiotoggle", Command_GPIO_Toggle, 1); // Gpio.h
      break;
    }
    case 'i': {
      COMMAND_CASE_R("i2cscanner", Command_i2c_Scanner, -1); // i2c.h
      COMMAND_CASE_R(        "ip", Command_IP,           1); // Network Command
      break;
    }
    case 'j': {
      #ifndef BUILD_NO_DIAGNOSTIC_COMMANDS
      COMMAND_CASE_A("jsonportstatus", Command_JSONPortStatus, -1); // Diagnostic.h
      #endif // ifndef BUILD_NO_DIAGNOSTIC_COMMANDS
      break;
    }
    case 'l': {
      COMMAND_CASE_A(            "let", Command_Rules_Let,         2); // Rules.h
      COMMAND_CASE_A(           "load", Command_Settings_Load,     0); // Settings.h
      COMMAND_CASE_A(       "logentry", Command_logentry,         -1); // Diagnostic.h
      COMMAND_CASE_A(   "looptimerset", Command_Loop_Timer_Set,    3); // Timers.h
      COMMAND_CASE_A("looptimerset_ms", Command_Loop_Timer_Set_ms, 3); // Timers.h
      COMMAND_CASE_A(      "longpulse", Command_GPIO_LongPulse,    5);    // GPIO.h
      COMMAND_CASE_A(   "longpulse_ms", Command_GPIO_LongPulse_Ms, 5);    // GPIO.h
    #ifndef BUILD_NO_DIAGNOSTIC_COMMANDS
      COMMAND_CASE_A(  "logportstatus", Command_logPortStatus,     0); // Diagnostic.h
      COMMAND_CASE_A(         "lowmem", Command_Lowmem,            0); // Diagnostic.h
    #endif // ifndef BUILD_NO_DIAGNOSTIC_COMMANDS
      break;
    }
    case 'm': {
#ifdef USES_P009
      if (cmd_lc_length > 3 && data.cmd_lc[3] == 'g') {
        COMMAND_CASE_A(        "mcpgpio", Command_GPIO,              2); // Gpio.h
        COMMAND_CASE_A(   "mcpgpiorange", Command_GPIO_McpGPIORange, -1); // Gpio.h
        COMMAND_CASE_A( "mcpgpiopattern", Command_GPIO_McpGPIOPattern, -1); // Gpio.h
        COMMAND_CASE_A(  "mcpgpiotoggle", Command_GPIO_Toggle,       1); // Gpio.h
      } else if (data.cmd_lc[1] == 'c') {
        COMMAND_CASE_A(   "mcplongpulse", Command_GPIO_LongPulse,    3); // GPIO.h
        COMMAND_CASE_A("mcplongpulse_ms", Command_GPIO_LongPulse_Ms, 3); // GPIO.h
        COMMAND_CASE_A(        "mcpmode", Command_GPIO_Mode,         2); // Gpio.h   
        COMMAND_CASE_A(   "mcpmoderange", Command_GPIO_ModeRange,    3); // Gpio.h   
        COMMAND_CASE_A(       "mcppulse", Command_GPIO_Pulse,        3); // GPIO.h
      }
#endif
      COMMAND_CASE_A(          "monitor", Command_GPIO_Monitor,      2); // GPIO.h
      COMMAND_CASE_A(     "monitorrange", Command_GPIO_MonitorRange, 3); // GPIO.h   
    #ifndef BUILD_NO_DIAGNOSTIC_COMMANDS
      COMMAND_CASE_A(          "malloc", Command_Malloc,         1);        // Diagnostic.h
      COMMAND_CASE_A(         "meminfo", Command_MemInfo,        0);        // Diagnostic.h
      COMMAND_CASE_A(   "meminfodetail", Command_MemInfo_detail, 0);        // Diagnostic.h
    #endif // ifndef BUILD_NO_DIAGNOSTIC_COMMANDS

      break;
    }
    case 'n': {
      COMMAND_CASE_R(   "name", Command_Settings_Name,        1); // Settings.h
      COMMAND_CASE_R("nosleep", Command_System_NoSleep,       1); // System.h
#if FEATURE_NOTIFIER
      COMMAND_CASE_R( "notify", Command_Notifications_Notify, 2); // Notifications.h
#endif
      COMMAND_CASE_R("ntphost", Command_NTPHost,              1); // Time.h
      break;
    }
    case 'p': {
#ifdef USES_P019
      if (cmd_lc_length > 3 && data.cmd_lc[3] == 'g') {
        COMMAND_CASE_A(        "pcfgpio", Command_GPIO,                 2); // Gpio.h
        COMMAND_CASE_A(   "pcfgpiorange", Command_GPIO_PcfGPIORange,   -1); // Gpio.h
        COMMAND_CASE_A( "pcfgpiopattern", Command_GPIO_PcfGPIOPattern, -1); // Gpio.h
        COMMAND_CASE_A(  "pcfgpiotoggle", Command_GPIO_Toggle,          1); // Gpio.h
      } else if (data.cmd_lc[1] == 'c') {
        COMMAND_CASE_A(   "pcflongpulse", Command_GPIO_LongPulse,       3); // GPIO.h
        COMMAND_CASE_A("pcflongpulse_ms", Command_GPIO_LongPulse_Ms,    3); // GPIO.h
        COMMAND_CASE_A(        "pcfmode", Command_GPIO_Mode,            2); // Gpio.h   
        COMMAND_CASE_A(   "pcfmoderange", Command_GPIO_ModeRange,       3); // Gpio.h   ************
        COMMAND_CASE_A(       "pcfpulse", Command_GPIO_Pulse,           3); // GPIO.h
      }
#endif
      COMMAND_CASE_R("password", Command_Settings_Password, 1); // Settings.h
#if FEATURE_CUSTOM_PROVISIONING
      COMMAND_CASE_A(       "provisionconfig", Command_Provisioning_Config,       0); // Provisioning.h
      COMMAND_CASE_A(     "provisionsecurity", Command_Provisioning_Security,     0); // Provisioning.h
      #if FEATURE_NOTIFIER
      COMMAND_CASE_A( "provisionnotification", Command_Provisioning_Notification, 0); // Provisioning.h
      #endif
      COMMAND_CASE_A(    "provisionprovision", Command_Provisioning_Provision,    0); // Provisioning.h
      COMMAND_CASE_A(        "provisionrules", Command_Provisioning_Rules,        1); // Provisioning.h
      COMMAND_CASE_A(     "provisionfirmware", Command_Provisioning_Firmware,     1); // Provisioning.h
#endif
      COMMAND_CASE_A(   "pulse", Command_GPIO_Pulse,        3); // GPIO.h
#if FEATURE_MQTT
      COMMAND_CASE_A( "publish", Command_MQTT_Publish,      2); // MQTT.h
#endif // if FEATURE_MQTT
      COMMAND_CASE_A(     "pwm", Command_GPIO_PWM,          4); // GPIO.h
      break;
    }
    case 'r': {
      COMMAND_CASE_A(                "reboot", Command_System_Reboot,              0); // System.h
      COMMAND_CASE_R(                 "reset", Command_Settings_Reset,             0); // Settings.h
      COMMAND_CASE_A("resetflashwritecounter", Command_RTC_resetFlashWriteCounter, 0); // RTC.h
      COMMAND_CASE_A(               "restart", Command_System_Reboot,              0); // System.h
      COMMAND_CASE_A(                 "rtttl", Command_GPIO_RTTTL,                -1); // GPIO.h
      COMMAND_CASE_A(                 "rules", Command_Rules_UseRules,             1); // Rule.h
      break;
    }
    case 's': {
      COMMAND_CASE_R(    "save", Command_Settings_Save, 0); // Settings.h
    #if FEATURE_SD
      COMMAND_CASE_R(  "sdcard", Command_SD_LS,         0); // SDCARDS.h
      COMMAND_CASE_R("sdremove", Command_SD_Remove,     1); // SDCARDS.h
    #endif // if FEATURE_SD

      if (data.cmd_lc[1] == 'e') {
      #if FEATURE_ESPEASY_P2P
        COMMAND_CASE_A(    "sendto", Command_UPD_SendTo,      2); // UDP.h    // FIXME TD-er: These send commands, can we determine the nr
                                                                  // of
                                                                  // arguments?
      #endif
        #if FEATURE_SEND_TO_HTTP
        COMMAND_CASE_A("sendtohttp", Command_HTTP_SendToHTTP, 3); // HTTP.h
        #endif // FEATURE_SEND_TO_HTTP
        COMMAND_CASE_A( "sendtoudp", Command_UDP_SendToUPD,   3); // UDP.h
    #ifndef BUILD_NO_DIAGNOSTIC_COMMANDS
        COMMAND_CASE_R("serialfloat", Command_SerialFloat,    0); // Diagnostic.h
    #endif // ifndef BUILD_NO_DIAGNOSTIC_COMMANDS
        COMMAND_CASE_R(   "settings", Command_Settings_Print, 0); // Settings.h
        COMMAND_CASE_A(      "servo", Command_Servo,          3); // Servo.h
      }
      COMMAND_CASE_A("status", Command_GPIO_Status,          2); // GPIO.h
      COMMAND_CASE_R("subnet", Command_Subnet, 1);                // Network Command
    #if FEATURE_MQTT
      COMMAND_CASE_A("subscribe", Command_MQTT_Subscribe, 1);     // MQTT.h
    #endif // if FEATURE_MQTT
    #ifndef BUILD_NO_DIAGNOSTIC_COMMANDS
      COMMAND_CASE_A(  "sysload", Command_SysLoad,        0);     // Diagnostic.h
    #endif // ifndef BUILD_NO_DIAGNOSTIC_COMMANDS
      break;
    }
    case 't': {
      if (data.cmd_lc[1] == 'a') {
        COMMAND_CASE_R(   "taskclear", Command_Task_Clear,    1);             // Tasks.h
        COMMAND_CASE_R("taskclearall", Command_Task_ClearAll, 0);             // Tasks.h
        COMMAND_CASE_R( "taskdisable", Command_Task_Disable,  1);             // Tasks.h
        COMMAND_CASE_R(  "taskenable", Command_Task_Enable,   1);             // Tasks.h
        COMMAND_CASE_A(           "taskrun", Command_Task_Run,            1); // Tasks.h
        COMMAND_CASE_A(      "taskvalueset", Command_Task_ValueSet,       3); // Tasks.h
        COMMAND_CASE_A(   "taskvaluetoggle", Command_Task_ValueToggle,    2); // Tasks.h
        COMMAND_CASE_A("taskvaluesetandrun", Command_Task_ValueSetAndRun, 3); // Tasks.h
      } else if (data.cmd_lc[1] == 'i') {
        COMMAND_CASE_A( "timerpause", Command_Timer_Pause,  1);               // Timers.h
        COMMAND_CASE_A("timerresume", Command_Timer_Resume, 1);               // Timers.h
        COMMAND_CASE_A(   "timerset", Command_Timer_Set,    2);               // Timers.h
        COMMAND_CASE_A("timerset_ms", Command_Timer_Set_ms, 2); // Timers.h
        COMMAND_CASE_R("timezone", Command_TimeZone, 1);                      // Time.h
      }
      COMMAND_CASE_A(      "tone", Command_GPIO_Tone, 3); // GPIO.h
      break;
    }
    case 'u': {
      COMMAND_CASE_R("udpport", Command_UDP_Port,      1);    // UDP.h
    #if FEATURE_ESPEASY_P2P
      COMMAND_CASE_R("udptest", Command_UDP_Test,      2);    // UDP.h
    #endif
      COMMAND_CASE_R(   "unit", Command_Settings_Unit, 1);    // Settings.h
      COMMAND_CASE_A("unmonitor", Command_GPIO_UnMonitor, 2); // GPIO.h
      COMMAND_CASE_A("unmonitorrange", Command_GPIO_UnMonitorRange, 3); // GPIO.h
      COMMAND_CASE_R("usentp", Command_useNTP, 1);            // Time.h
      break;
    }
    case 'w': {
      #ifndef LIMIT_BUILD_SIZE
      COMMAND_CASE_R("wdconfig", Command_WD_Config, 3);               // WD.h
      COMMAND_CASE_R(  "wdread", Command_WD_Read,   2);               // WD.h
      #endif

      if (data.cmd_lc[1] == 'i') {
        COMMAND_CASE_R(   "wifiallowap", Command_Wifi_AllowAP,    0); // WiFi.h
        COMMAND_CASE_R(    "wifiapmode", Command_Wifi_APMode,     0); // WiFi.h
        COMMAND_CASE_A(   "wificonnect", Command_Wifi_Connect,    0); // WiFi.h
        COMMAND_CASE_A("wifidisconnect", Command_Wifi_Disconnect, 0); // WiFi.h
        COMMAND_CASE_R(       "wifikey", Command_Wifi_Key,        1); // WiFi.h
        COMMAND_CASE_R(      "wifikey2", Command_Wifi_Key2,       1); // WiFi.h
        COMMAND_CASE_R(      "wifimode", Command_Wifi_Mode,       1); // WiFi.h
        COMMAND_CASE_R(      "wifiscan", Command_Wifi_Scan,       0); // WiFi.h
        COMMAND_CASE_R(      "wifissid", Command_Wifi_SSID,       1); // WiFi.h
        COMMAND_CASE_R(     "wifissid2", Command_Wifi_SSID2,      1); // WiFi.h
        COMMAND_CASE_R(   "wifistamode", Command_Wifi_STAMode,    0); // WiFi.h
      }
      break;
    }
    default:
      break;
  }

  #undef COMMAND_CASE_R
  #undef COMMAND_CASE_A
  return false;
}

// Execute command which may be plugin or internal commands
bool ExecuteCommand_all(EventValueSource::Enum source, const char *Line)
{
  return ExecuteCommand(INVALID_TASK_INDEX, source, Line, true, true, false);
}

bool ExecuteCommand_all_config(EventValueSource::Enum source, const char *Line)
{
  return ExecuteCommand(INVALID_TASK_INDEX, source, Line, true, true, true);
}

bool ExecuteCommand_plugin_config(EventValueSource::Enum source, const char *Line)
{
  return ExecuteCommand(INVALID_TASK_INDEX, source, Line, true, false, true);
}

bool ExecuteCommand_all_config_eventOnly(EventValueSource::Enum source, const char *Line)
{
  bool tryInternal = false;
  {
    String cmd;

    if (GetArgv(Line, cmd, 1)) {
      tryInternal = cmd.equalsIgnoreCase(F("event"));
    }
  }

  return ExecuteCommand(INVALID_TASK_INDEX, source, Line, true, tryInternal, true);
}

bool ExecuteCommand_internal(EventValueSource::Enum source, const char *Line)
{
  return ExecuteCommand(INVALID_TASK_INDEX, source, Line, false, true, false);
}

bool ExecuteCommand_plugin(EventValueSource::Enum source, const char *Line)
{
  return ExecuteCommand(INVALID_TASK_INDEX, source, Line, true, false, false);
}

bool ExecuteCommand_plugin(taskIndex_t taskIndex, EventValueSource::Enum source, const char *Line)
{
  return ExecuteCommand(taskIndex, source, Line, true, false, false);
}

bool ExecuteCommand(taskIndex_t            taskIndex,
                    EventValueSource::Enum source,
                    const char            *Line,
                    bool                   tryPlugin,
                    bool                   tryInternal,
                    bool                   tryRemoteConfig)
{
  #ifndef BUILD_NO_RAM_TRACKER
  checkRAM(F("ExecuteCommand"));
  #endif
  String cmd;

  // We first try internal commands, which should not have a taskIndex set.
  struct EventStruct TempEvent;

  if (!GetArgv(Line, cmd, 1)) {
    SendStatus(&TempEvent, return_command_failed());
    return false;
  }

  if (tryInternal) {
    // Small optimization for events, which happen frequently
    // FIXME TD-er: Make quick check to see if a command is an internal command, so we don't need to try all
    if (cmd.equalsIgnoreCase(F("event"))) {
      tryPlugin       = false;
      tryRemoteConfig = false;
    }
  }

  TempEvent.Source = source;

  String action(Line);
  action = parseTemplate(action); // parseTemplate before executing the command

  // Split the arguments into Par1...5 of the event.
  // Do not split it in executeInternalCommand, since that one will be called from the scheduler with pre-set events.
  // FIXME TD-er: Why call this for all commands? The CalculateParam function is quite heavy.
  parseCommandString(&TempEvent, action);

  // FIXME TD-er: This part seems a bit strange.
  // It can't schedule a call to PLUGIN_WRITE.
  // Maybe ExecuteCommand can be scheduled?
  delay(0);

#ifndef BUILD_NO_DEBUG
  if (loglevelActiveFor(LOG_LEVEL_DEBUG)) {
    {
      String log = F("Command: ");
      log += cmd;
      addLogMove(LOG_LEVEL_DEBUG, log);
    }
    addLog(LOG_LEVEL_DEBUG, Line); // for debug purposes add the whole line.
    {
      String parameters;
      parameters.reserve(64);
      parameters += F("Par1: ");
      parameters += TempEvent.Par1;
      parameters += F(" Par2: ");
      parameters += TempEvent.Par2;
      parameters += F(" Par3: ");
      parameters += TempEvent.Par3;
      parameters += F(" Par4: ");
      parameters += TempEvent.Par4;
      parameters += F(" Par5: ");
      parameters += TempEvent.Par5;
      addLogMove(LOG_LEVEL_DEBUG, parameters);
    }
  }
#endif // ifndef BUILD_NO_DEBUG


  if (tryInternal) {
    command_case_data data(cmd.c_str(), &TempEvent, action.c_str());
    bool   handled = executeInternalCommand(data);

    if (data.status.length() > 0) {
      delay(0);
      SendStatus(&TempEvent, data.status);
      delay(0);
    }

    if (handled) {
//      addLog(LOG_LEVEL_INFO, F("executeInternalCommand accepted"));
      return true;
    }
  }

  // When trying a task command, set the task index, even if it is not a valid task index.
  // For example commands from elsewhere may not have a proper task index.
  TempEvent.setTaskIndex(taskIndex);
  checkDeviceVTypeForTask(&TempEvent);

  if (tryPlugin) {
    // Use a tmp string to call PLUGIN_WRITE, since PluginCall may inadvertenly
    // alter the string.
    String tmpAction(action);
    bool   handled = PluginCall(PLUGIN_WRITE, &TempEvent, tmpAction);
//    if (handled) addLog(LOG_LEVEL_INFO, F("PLUGIN_WRITE accepted"));
    
    #ifndef BUILD_NO_DEBUG
    if (!tmpAction.equals(action)) {
      if (loglevelActiveFor(LOG_LEVEL_ERROR)) {
        String log = F("PLUGIN_WRITE altered the string: ");
        log += action;
        log += F(" to: ");
        log += tmpAction;
        addLogMove(LOG_LEVEL_ERROR, log);
      }
    }
    #endif

    if (!handled) {
      // Try a controller
      handled = CPluginCall(CPlugin::Function::CPLUGIN_WRITE, &TempEvent, tmpAction);
//      if (handled) addLog(LOG_LEVEL_INFO, F("CPLUGIN_WRITE accepted"));
    }

    if (handled) {
      SendStatus(&TempEvent, return_command_success());
      return true;
    }
  }

  if (tryRemoteConfig) {
    if (remoteConfig(&TempEvent, action)) {
      SendStatus(&TempEvent, return_command_success());
//      addLog(LOG_LEVEL_INFO, F("remoteConfig accepted"));

      return true;
    }
  }
  String errorUnknown = F("Command unknown: ");
  errorUnknown += action;
  addLog(LOG_LEVEL_INFO, errorUnknown);
  SendStatus(&TempEvent, errorUnknown);
  delay(0);
  return false;
}

#include "../Commands/wd.h"

#ifndef LIMIT_BUILD_SIZE

#include "../Commands/Common.h"

#include "../DataStructs/ESPEasy_EventStruct.h"

#include "../ESPEasyCore/Serial.h"

#include "../Helpers/StringConverter.h"


const __FlashStringHelper * Command_WD_Config(EventStruct *event, const char* Line)
{
  Wire.beginTransmission(event->Par1);  // address
  Wire.write(event->Par2);              // command
  Wire.write(event->Par3);              // data
  Wire.endTransmission();
  return return_command_success();
}

String Command_WD_Read(EventStruct *event, const char* Line)
{
  Wire.beginTransmission(event->Par1);  // address
  Wire.write(0x83);                     // command to set pointer
  Wire.write(event->Par2);              // pointer value
  Wire.endTransmission();
  if ( Wire.requestFrom(static_cast<uint8_t>(event->Par1), static_cast<uint8_t>(1)) == 1 )
  {
    uint8_t value = Wire.read();
    return return_result(
      event, 
      concat(F("I2C Read address "),  formatToHex(event->Par1)) 
    + concat(F(" Value "), formatToHex(value)));
  }
  return return_command_success_str();
}

#endif
#include "../Commands/Provisioning.h"

#if FEATURE_CUSTOM_PROVISIONING

# include "../Commands/Common.h"
# include "../DataTypes/ESPEasyFileType.h"
# include "../DataStructs/ESPEasy_EventStruct.h"
# include "../Helpers/ESPEasy_Storage.h"
# include "../Helpers/Networking.h"
# include "../Helpers/StringConverter.h"

String Command_Provisioning_Config(struct EventStruct *event, const char *Line)
{
  return downloadFileType(FileType::CONFIG_DAT);
}

String Command_Provisioning_Security(struct EventStruct *event, const char *Line)
{
  return downloadFileType(FileType::SECURITY_DAT);
}

#if FEATURE_NOTIFIER
String Command_Provisioning_Notification(struct EventStruct *event, const char *Line)
{
  return downloadFileType(FileType::NOTIFICATION_DAT);
}
#endif

String Command_Provisioning_Provision(struct EventStruct *event, const char *Line)
{
  return downloadFileType(FileType::PROVISIONING_DAT);
}

String Command_Provisioning_Rules(struct EventStruct *event, const char *Line)
{
  if ((event->Par1 <= 0) || (event->Par1 > 4)) {
    return F("ProvisionRules: rules index out of range");
  }
  return downloadFileType(FileType::RULES_TXT, event->Par1 - 1);
}

String Command_Provisioning_Firmware(struct EventStruct *event, const char *Line)
{
  const String url = parseStringToEndKeepCase(Line, 2);
  String error;
  if (downloadFirmware(url, error)) {
    // TODO TD-er: send events
  }
  return error;
}


#endif // if FEATURE_CUSTOM_PROVISIONING

#include "../Commands/Timer.h"




#include "../../ESPEasy_common.h"


#include "../Commands/Common.h"

#include "../ESPEasyCore/ESPEasy_Log.h"
#include "../ESPEasyCore/ESPEasyRules.h"

#include "../Globals/ESPEasy_Scheduler.h"

#include "../Helpers/ESPEasy_time_calc.h"
#include "../Helpers/Misc.h"
#include "../Helpers/Scheduler.h"

const __FlashStringHelper * command_setRulesTimer(int msecFromNow, int timerIndex, int recurringCount) {
  if (msecFromNow < 0)
  {
    addLog(LOG_LEVEL_ERROR, F("TIMER: time must be positive"));
  } else {
    // start new timer when msecFromNow > 0
    // Clear timer when msecFromNow == 0
    if (Scheduler.setRulesTimer(msecFromNow, timerIndex, recurringCount))
    { 
      return return_command_success();
    }
  }
  return return_command_failed();
}

const __FlashStringHelper * Command_Timer_Set(struct EventStruct *event, const char *Line)
{
  return command_setRulesTimer(
    event->Par2 * 1000, // msec from now
    event->Par1,        // timer index
    0                   // recurringCount
    );
}

const __FlashStringHelper * Command_Timer_Set_ms (struct EventStruct *event, const char* Line)
{
  return command_setRulesTimer(
    event->Par2, // interval
    event->Par1, // timer index
    0            // recurringCount
    );
}

const __FlashStringHelper * Command_Loop_Timer_Set (struct EventStruct *event, const char* Line)
{
  int recurringCount = event->Par3;
  if (recurringCount == 0) {
    // if the optional 3rd parameter is not given, set it to "run always"
    recurringCount = -1;
  }
  return command_setRulesTimer(
    event->Par2 * 1000, // msec from now
    event->Par1,        // timer index
    recurringCount
    );
}

const __FlashStringHelper * Command_Loop_Timer_Set_ms (struct EventStruct *event, const char* Line)
{
  int recurringCount = event->Par3;
  if (recurringCount == 0) {
    // if the optional 3rd parameter is not given, set it to "run always"
    recurringCount = -1;
  }
  return command_setRulesTimer(
    event->Par2, // interval
    event->Par1, // timer index
    recurringCount
    );
}

const __FlashStringHelper * Command_Timer_Pause(struct EventStruct *event, const char *Line)
{
  if (Scheduler.pause_rules_timer(event->Par1)) {
    String eventName = F("Rules#TimerPause=");
    eventName += event->Par1;
    rulesProcessing(eventName); // TD-er: Process right now
    return return_command_success();
  }
  return return_command_failed();
}

const __FlashStringHelper * Command_Timer_Resume(struct EventStruct *event, const char *Line)
{
  if (Scheduler.resume_rules_timer(event->Par1)) {
    String eventName = F("Rules#TimerResume=");
    eventName += event->Par1;
    rulesProcessing(eventName); // TD-er: Process right now
    return return_command_success();
  }
  return return_command_failed();
}

const __FlashStringHelper * Command_Delay(struct EventStruct *event, const char *Line)
{
  delayBackground(event->Par1);
  return return_command_success();
}

#include "../Commands/GPIO.h"

#include "../../ESPEasy_common.h"


#include "../../ESPEasy-Globals.h"

#include "../Commands/Common.h"
#include "../DataStructs/PinMode.h"
#include "../ESPEasyCore/Controller.h"
#include "../ESPEasyCore/ESPEasyGPIO.h"
#include "../ESPEasyCore/ESPEasy_Log.h"
#include "../Globals/ESPEasy_Scheduler.h"
#include "../Globals/GlobalMapPortStatus.h"
#include "../Helpers/Audio.h"
#include "../Helpers/Hardware.h"
#include "../Helpers/StringConverter.h"
#include "../Helpers/PortStatus.h"
#include "../Helpers/Numerical.h"

#if FEATURE_GPIO_USE_ESP8266_WAVEFORM
# include <core_esp8266_waveform.h>
#endif 

// Forward declarations of functions used in this module
// Normally those would be declared in the .h file as private members
// But since these are not part of a class, forward declare them in the .cpp
//void createAndSetPortStatus_Mode_State(uint32_t key, uint8_t newMode, int8_t newState);
const __FlashStringHelper * getPluginIDAndPrefix(char selection, pluginID_t& pluginID, bool& success);
void logErrorGpioOffline(const __FlashStringHelper * prefix, int port);
void logErrorGpioOutOfRange(const __FlashStringHelper * prefix, int port, const char* Line = nullptr);
void logErrorGpioNotOutput(const __FlashStringHelper * prefix, int port);
void logErrorModeOutOfRange(const __FlashStringHelper * prefix, int port);
bool gpio_monitor_helper(int port, struct EventStruct *event, const char* Line);
bool gpio_unmonitor_helper(int port, struct EventStruct *event, const char* Line);
#ifdef USES_P009
bool mcpgpio_range_pattern_helper(struct EventStruct *event, const char* Line, bool isWritePattern);
#endif
#ifdef USES_P019
bool pcfgpio_range_pattern_helper(struct EventStruct *event, const char* Line, bool isWritePattern);
#endif
bool gpio_mode_range_helper(uint8_t pin, uint8_t pinMode, struct EventStruct *event, const char* Line);
#ifdef USES_P019
uint8_t getPcfAddress(uint8_t pin);
#endif
bool setGPIOMode(uint8_t pin, uint8_t mode);
#ifdef USES_P019
bool setPCFMode(uint8_t pin, uint8_t mode);
#endif
#ifdef USES_P009
bool setMCPMode(uint8_t pin, uint8_t mode);
bool mcpgpio_plugin_range_helper(uint8_t pin1, uint8_t pin2, uint16_t &result);
#endif
#ifdef USES_P019
bool pcfgpio_plugin_range_helper(uint8_t pin1, uint8_t pin2, uint16_t &result);
#endif


/*************************************************************************/

const __FlashStringHelper * Command_GPIO_Monitor(struct EventStruct *event, const char *Line)
{
  if (gpio_monitor_helper(event->Par2, event, Line)) {
    return return_command_success();
  }
  return return_command_failed();
}

const __FlashStringHelper * Command_GPIO_MonitorRange(struct EventStruct *event, const char *Line)
{
  bool success = true;

  for (uint8_t i = event->Par2; i <= event->Par3; i++) {
    success &= gpio_monitor_helper(i, event, Line);
  }
  return success ? return_command_success() : return_command_failed();
}

bool gpio_monitor_helper(int port, struct EventStruct *event, const char *Line)
{
  pluginID_t pluginID = INVALID_PLUGIN_ID;
  bool success = false;

  // parseString(Line, 2).charAt(0)='g':gpio; ='p':pcf; ='m':mcp
  const __FlashStringHelper * logPrefix = getPluginIDAndPrefix(parseString(Line, 2).charAt(0), pluginID, success);

  if (success && checkValidPortRange(pluginID, port))
  {
    const uint32_t key = createKey(pluginID, port); // WARNING: 'monitor' uses Par2 instead of Par1
    // if (!existPortStatus(key)) globalMapPortStatus[key].mode=PIN_MODE_OUTPUT;
    addMonitorToPort(key);

    int8_t state;

    // giig1967g: Comment next 3 lines to receive an EVENT just after calling the monitor command
    GPIO_Read(pluginID, port, state);
    globalMapPortStatus[key].state = state;

    if (state == -1) { globalMapPortStatus[key].mode = PIN_MODE_OFFLINE; }

    if (loglevelActiveFor(LOG_LEVEL_INFO)) {
      String log = concat(logPrefix,  F(" port #")); 
      log += port; 
      log += F(": added to monitor list.");
      addLogMove(LOG_LEVEL_INFO, log);
    }
    String dummy;
    SendStatusOnlyIfNeeded(event, SEARCH_PIN_STATE, key, dummy, 0);

    return true;
  } else {
    logErrorGpioOutOfRange(logPrefix, port, Line);
    return false;
  }
}

const __FlashStringHelper * Command_GPIO_UnMonitor(struct EventStruct *event, const char *Line)
{
  if (gpio_unmonitor_helper(event->Par2, event, Line)) {
    return return_command_success();
  }
  else {
    return return_command_failed();
  }
}

const __FlashStringHelper * Command_GPIO_UnMonitorRange(struct EventStruct *event, const char *Line)
{
  bool success = true;

  for (uint8_t i = event->Par2; i <= event->Par3; i++) {
    success &= gpio_unmonitor_helper(i, event, Line);
  }
  return success ? return_command_success() : return_command_failed();
}

bool gpio_unmonitor_helper(int port, struct EventStruct *event, const char *Line)
{

  pluginID_t pluginID = INVALID_PLUGIN_ID;
  bool success = false;

  // parseString(Line, 2).charAt(0)='g':gpio; ='p':pcf; ='m':mcp
  const __FlashStringHelper * logPrefix = getPluginIDAndPrefix(parseString(Line, 2).charAt(0), pluginID, success);

  if (success && checkValidPortRange(pluginID, port))
  {
    const uint32_t key = createKey(pluginID, port); // WARNING: 'monitor' uses Par2 instead of Par1
    String dummy;
    SendStatusOnlyIfNeeded(event, SEARCH_PIN_STATE, key, dummy, 0);

    removeMonitorFromPort(key);
    if (loglevelActiveFor(LOG_LEVEL_INFO)) {
      String log = concat(logPrefix, F(" port #"));
      log += port;
      log += F(": removed from monitor list.");
      addLogMove(LOG_LEVEL_INFO, log);
    }

    return true;
  } else {
    logErrorGpioOutOfRange(logPrefix, port, Line);
    return false;
  }
}

const __FlashStringHelper * Command_GPIO_LongPulse(struct EventStruct *event, const char *Line)
{
  event->Par3 *= 1000;
  event->Par4 *= 1000;
  event->Par5 *= 1000;
  return Command_GPIO_LongPulse_Ms(event, Line);
}

const __FlashStringHelper * Command_GPIO_LongPulse_Ms(struct EventStruct *event, const char *Line)
{
  pluginID_t pluginID = INVALID_PLUGIN_ID;
  bool success = false;

  // Line[0]='l':longpulse; ='p':pcflongpulse; ='m':mcplongpulse
  const __FlashStringHelper * logPrefix = getPluginIDAndPrefix(Line[0], pluginID, success);

  if (success && checkValidPortRange(pluginID, event->Par1))
  {
    // Event parameters
    // Par1: pin nr
    // Par2: state
    // Par3: duration of  state
    // Par4: duration of !state
    // Par5: repeat count (only when Par4 > 0)
    //       -1 = repeat indefinately
    //        0 = only once
    //        N = Repeat N times
    Scheduler.clearGPIOTimer(pluginID, event->Par1);
    const uint32_t key = createKey(pluginID, event->Par1);
    createAndSetPortStatus_Mode_State(key, PIN_MODE_OUTPUT, event->Par2);

    #if FEATURE_GPIO_USE_ESP8266_WAVEFORM
    bool usingWaveForm = 
        event->Par3 > 0 && event->Par3 < 15000 &&
        event->Par4 > 0 && event->Par4 < 15000 &&
        event->Par5 != 0;
        
    if (usingWaveForm) {
      // Preferred is to use the ESP8266 waveform function.
      // Max time high or low is roughly 53 sec @ 80 MHz or half @160 MHz.
      // This is not available on ESP32.

      const uint8_t pin = event->Par1;
      uint32_t timeHighUS = event->Par3 * 1000;
      uint32_t timeLowUS  = event->Par4 * 1000;

      if (event->Par2 == 0) {
        std::swap(timeHighUS, timeLowUS);
      }
      uint32_t runTimeUS = 0;
      if (event->Par5 > 0) {
        // Must set slightly lower than expected duration as it will be rounded up.
        runTimeUS = event->Par5 * (timeHighUS + timeLowUS) - ((timeHighUS + timeLowUS) / 2);
      }

      pinMode(event->Par1, OUTPUT);
      usingWaveForm = startWaveform(
        pin, timeHighUS, timeLowUS, runTimeUS);
    }
    #else
    // waveform function not available on ESP32
    const bool usingWaveForm = false;
    #endif

    if (!usingWaveForm) {
      // Par1 = pinnr
      // Par2 = pin state
      // Par3 = timeHigh in msec
      // Par4 = timeLow in msec
      // Par5 = repeat count
      GPIO_Write(pluginID, event->Par1, event->Par2);
      if (event->Par4 > 0 && event->Par5 != 0) {
        // Compute repeat interval

        // Schedule switching pin to given state for repeat
        Scheduler.setGPIOTimer(
          event->Par3,   // msecFromNow
          pluginID,    
          event->Par1,   // Pin/port nr
          event->Par2,   // pin state
          event->Par3,   // repeat interval (high)
          event->Par5,   // repeat count
          event->Par4);  // alternate interval (low)
      } else {
        // Schedule switching pin back to original state
        Scheduler.setGPIOTimer(
          event->Par3,   // msecFromNow
          pluginID,    
          event->Par1,   // Pin/port nr
          !event->Par2,  // pin state
          0, // repeatInterva
          0); // repeatCount
      }
    }


    String log = logPrefix;
    log += F(" : port ");
    log += event->Par1;
    log += F(". Pulse H:");
    log += event->Par3;
    if (event->Par4 > 0 && event->Par5 != 0) {
      log += F(" L:");
      log += event->Par4;
      log += F(" #:");
      log += event->Par5;
    }
    log += F(" ms");
    addLog(LOG_LEVEL_INFO, log);
    SendStatusOnlyIfNeeded(event, SEARCH_PIN_STATE, key, log, 0);

    return return_command_success();
  } else {
    logErrorGpioOutOfRange(logPrefix, event->Par1, Line);
    return return_command_failed();
  }
}

const __FlashStringHelper * Command_GPIO_Status(struct EventStruct *event, const char *Line)
{
  bool sendStatusFlag;
  uint8_t pluginID = 0;

  switch (tolower(parseString(Line, 2).charAt(0)))
  {
    case 'g': // gpio
      pluginID       = PLUGIN_GPIO;
      sendStatusFlag = true;
      break;
#ifdef USES_P009
    case 'm': // mcp
      pluginID       = PLUGIN_MCP;
      sendStatusFlag = GPIO_MCP_Read(event->Par2) == -1;
      break;
#endif
#ifdef USES_P019
    case 'p': // pcf
      pluginID       = PLUGIN_PCF;
      sendStatusFlag = GPIO_PCF_Read(event->Par2) == -1;
      break;
#endif
    default:
      addLog(LOG_LEVEL_ERROR, F("Plugin not included in build"));
      return return_command_failed();
  }

  if (!checkValidPortRange(pluginID, event->Par2))
  {
    return return_command_failed();
  }
  const uint32_t key = createKey(pluginID, event->Par2); // WARNING: 'status' uses Par2 instead of Par1
  String dummy;
  SendStatusOnlyIfNeeded(event, sendStatusFlag, key, dummy, 0);
  return return_command_success();
}

const __FlashStringHelper * Command_GPIO_PWM(struct EventStruct *event, const char *Line)
{
  // Par1: GPIO
  // Par2: Duty Cycle
  // Par3: Fade duration
  // Par4: Frequency

  // For now, we only support the internal GPIO pins.
  const __FlashStringHelper * logPrefix = F("GPIO");
  uint32_t frequency = event->Par4;
  uint32_t key       = 0;

  if (set_Gpio_PWM(event->Par1, event->Par2, event->Par3, frequency, key)) {
    String log = F("PWM  : GPIO: ");
    log += event->Par1;
    log += F(" duty: ");
    log += event->Par2;

    if (event->Par3 != 0) {
      log += F(" Fade: ");
      log += event->Par3;
      log += F(" ms");
    }

    if (event->Par4 != 0) {
      log += F(" f: ");
      log += frequency;
      log += F(" Hz");
    }
    addLog(LOG_LEVEL_INFO, log);
    SendStatusOnlyIfNeeded(event, SEARCH_PIN_STATE, key, log, 0);

    // SendStatus(event, getPinStateJSON(SEARCH_PIN_STATE, pluginID, event->Par1, log, 0));

    return return_command_success();
  }
  logErrorGpioOutOfRange(logPrefix, event->Par1, Line);
  return return_command_failed();
}

const __FlashStringHelper * Command_GPIO_Tone(struct EventStruct *event, const char *Line)
{
  // play a tone on pin par1, with frequency par2 and duration in msec par3.
  unsigned long duration   = event->Par3;
  bool mustScheduleToneOff = false;

  if (duration > 50) {
    duration            = 0;
    mustScheduleToneOff = true;
  }

  if (tone_espEasy(event->Par1, event->Par2, duration)) {
    if (mustScheduleToneOff) {
      // For now, we only support the internal GPIO pins.
      uint8_t pluginID = PLUGIN_GPIO;
      Scheduler.setGPIOTimer(event->Par3, pluginID, event->Par1, 0);
    }
    return return_command_success();
  }
  return return_command_failed();
}

const __FlashStringHelper * Command_GPIO_RTTTL(struct EventStruct *event, const char *Line)
{
  #if FEATURE_RTTTL

  // FIXME: Absolutely no error checking in play_rtttl, until then keep it only in testing
  // play a tune via a RTTTL string, look at https://www.letscontrolit.com/forum/viewtopic.php?f=4&t=343&hilit=speaker&start=10 for
  // more info.

  String melody = parseStringToEndKeepCase(Line, 2);
  melody.replace('-', '#');

  if (loglevelActiveFor(LOG_LEVEL_INFO)) {
    String log = concat(F("RTTTL : pin: "),  event->Par1);
    log += F(" melody: ");
    log += melody;
    addLogMove(LOG_LEVEL_INFO, log);
  }

  if (play_rtttl(event->Par1, melody.c_str())) {
    return return_command_success();
  }
  #else // if FEATURE_RTTTL
  addLog(LOG_LEVEL_ERROR, F("RTTTL : command not included in build"));
  #endif // if FEATURE_RTTTL
  return return_command_failed();
}

const __FlashStringHelper * Command_GPIO_Pulse(struct EventStruct *event, const char *Line)
{
  const __FlashStringHelper * logPrefix = F("");
  bool   success  = false;
  uint8_t   pluginID = INVALID_PLUGIN_ID;

  switch (tolower(Line[0]))
  {
    case 'p':                        // pulse or pcfpulse

      if (tolower(Line[1]) == 'u') { // pulse
        pluginID  = PLUGIN_GPIO;
        logPrefix = F("GPIO");
        success   = true;
      } else if (tolower(Line[1]) == 'c') { // pcfpulse
        pluginID  = PLUGIN_PCF;
        logPrefix = F("PCF");
        success   = true;
      }
      break;
    case 'm': // mcp
      pluginID  = PLUGIN_MCP;
      logPrefix = F("MCP");
      success   = true;
      break;
  }

  if (success && checkValidPortRange(pluginID, event->Par1))
  {
    const uint32_t key = createKey(pluginID, event->Par1);

    createAndSetPortStatus_Mode_State(key, PIN_MODE_OUTPUT, event->Par2);
    GPIO_Write(pluginID, event->Par1, event->Par2);

    delay(event->Par3);

    createAndSetPortStatus_Mode_State(key, PIN_MODE_OUTPUT, !event->Par2);
    GPIO_Write(pluginID, event->Par1, !event->Par2);

    String log;
    log += logPrefix;
    log += concat(F(" : port "),  event->Par1);
    log += concat(F(". Pulse set for "),  event->Par3);
    log += F(" ms");
    addLog(LOG_LEVEL_INFO, log);
    SendStatusOnlyIfNeeded(event, SEARCH_PIN_STATE, key, log, 0);

    return return_command_success();
  } else {
    logErrorGpioOutOfRange(logPrefix, event->Par1, Line);
    return return_command_failed();
  }
}

const __FlashStringHelper * Command_GPIO_Toggle(struct EventStruct *event, const char *Line)
{
  pluginID_t pluginID = INVALID_PLUGIN_ID;
  bool success = false;

  // Line[0]='g':gpiotoggle; ='p':pcfgpiotoggle; ='m':mcpgpiotoggle
  const __FlashStringHelper * logPrefix = getPluginIDAndPrefix(Line[0], pluginID, success);

  if (success && checkValidPortRange(pluginID, event->Par1))
  {
    const uint32_t key = createKey(pluginID, event->Par1);

    // WARNING: operator [] creates an entry in the map if key does not exist
    // So the next command should be part of each command:
    uint8_t   mode;
    int8_t state;

    auto it = globalMapPortStatus.find(key);

    if (it != globalMapPortStatus.end()) {
      mode  = it->second.mode;
      state = it->second.state;
    } else {
      GPIO_Read(pluginID, event->Par1, state);
      mode = (state == -1) ? PIN_MODE_OFFLINE : PIN_MODE_OUTPUT;
    }

    switch (mode) {
      case PIN_MODE_OUTPUT:
      case PIN_MODE_UNDEFINED:
      {
        createAndSetPortStatus_Mode_State(key, PIN_MODE_OUTPUT, !state);
        GPIO_Write(pluginID, event->Par1, !state);

        String log = logPrefix;
        log += F(" toggle: port#");
        log += event->Par1;
        log += F(": set to ");
        log += static_cast<int>(!state);
        addLog(LOG_LEVEL_ERROR, log);
        SendStatusOnlyIfNeeded(event, SEARCH_PIN_STATE, key, log, 0);

        return return_command_success();
        break;
      }
      case PIN_MODE_OFFLINE:
        logErrorGpioOffline(logPrefix, event->Par1);
        return return_command_failed();
      default:
        logErrorGpioNotOutput(logPrefix, event->Par1);
        return return_command_failed();
    }
  } else {
    logErrorGpioOutOfRange(logPrefix, event->Par1, Line);
    return return_command_failed();
  }
}

const __FlashStringHelper * Command_GPIO(struct EventStruct *event, const char *Line)
{
  pluginID_t pluginID = INVALID_PLUGIN_ID;
  bool success = false;

  // Line[0]='g':gpio; ='p':pcfgpio; ='m':mcpgpio
  const __FlashStringHelper * logPrefix = getPluginIDAndPrefix(Line[0], pluginID, success);

  if (success && checkValidPortRange(pluginID, event->Par1))
  {
    int8_t state = 0;
    uint8_t   mode;

    if (event->Par2 == 2) { // INPUT
      mode = PIN_MODE_INPUT_PULLUP;

      switch (pluginID) {
        case PLUGIN_GPIO:
          setInternalGPIOPullupMode(event->Par1);
          state = GPIO_Read_Switch_State(event->Par1, PIN_MODE_INPUT_PULLUP);
          break;
#ifdef USES_P009
        case PLUGIN_MCP:
          setMCPInputAndPullupMode(event->Par1, true);
          GPIO_Read(PLUGIN_MCP, event->Par1, state);
          break;
#endif
#ifdef USES_P019
        case PLUGIN_PCF:
          // PCF8574 specific: only can read 0/low state, so we must send 1
          state = 1;
          break;
#endif
        default:
          addLog(LOG_LEVEL_ERROR, F("Plugin not included in build"));
          return return_command_failed();
      }
    } else { // OUTPUT
      mode  = PIN_MODE_OUTPUT;
      state = (event->Par2 == 0) ? 0 : 1;
    }

    const uint32_t key = createKey(pluginID, event->Par1);

    if (globalMapPortStatus[key].mode != PIN_MODE_OFFLINE)
    {
      int8_t currentState;
      GPIO_Read(pluginID, event->Par1, currentState);

      if (currentState == -1) {
        mode  = PIN_MODE_OFFLINE;
        state = -1;
      }

      createAndSetPortStatus_Mode_State(key, mode, state);

      if ((mode == PIN_MODE_OUTPUT) || (pluginID == PLUGIN_PCF)) { GPIO_Write(pluginID, event->Par1, state, mode); }

      String log = logPrefix;
      log += F(" : port#");
      log += event->Par1;
      log += F(": set to ");
      log += state;
      addLog(LOG_LEVEL_INFO, log);
      SendStatusOnlyIfNeeded(event, SEARCH_PIN_STATE, key, log, 0);
      return return_command_success();
    } else {
      logErrorGpioOffline(logPrefix, event->Par1);
      return return_command_failed();
    }
  } else {
    logErrorGpioOutOfRange(logPrefix, event->Par1, Line);
    return return_command_failed();
  }
}

void logErrorGpio(const __FlashStringHelper * prefix, int port, const __FlashStringHelper * description)
{
  if (port >= 0) {
    addLog(LOG_LEVEL_ERROR, concat(prefix, F(" : port#")) + String(port) + description);
  }
}

void logErrorModeOutOfRange(const __FlashStringHelper * prefix, int port)
{
  logErrorGpio(prefix, port, F(" mode selection is incorrect. Valid values are: 0, 1 or 2."));
}

void logErrorGpioOffline(const __FlashStringHelper * prefix, int port)
{
  logErrorGpio(prefix, port, F(" is offline."));
}

void logErrorGpioOutOfRange(const __FlashStringHelper * prefix, int port, const char *Line)
{
  logErrorGpio(prefix, port, F(" is out of range"));
  # ifndef BUILD_NO_DEBUG
  if (port >= 0) {
    if (Line != nullptr) {
      addLog(LOG_LEVEL_DEBUG, Line);
    }
  }
  #endif
}

void logErrorGpioNotOutput(const __FlashStringHelper * prefix, int port)
{
  logErrorGpio(prefix, port, F(" is not an output port"));
}

void createAndSetPortStatus_Mode_State(uint32_t key, uint8_t newMode, int8_t newState)
{
  // WARNING: operator [] creates an entry in the map if key does not exist

  #ifdef ESP32
  switch (newMode) {
    case PIN_MODE_PWM:
    case PIN_MODE_SERVO:
      break;
    default:
      checkAndClearPWM(key);
      break;
  }
  #endif


  // If it doesn't exist, it is now created.
  globalMapPortStatus[key].mode = newMode;
  auto it = globalMapPortStatus.find(key);

  if (it != globalMapPortStatus.end()) {
    // Should always be true, as it would be created if it didn't exist.
    it->second.command = 1; // set to 1 in order to display the status in the PinStatus page

    // only force events if state has changed
    if (it->second.state != newState) {
      it->second.state        = newState;
      it->second.output       = newState;
      it->second.forceEvent   = 1;
      it->second.forceMonitor = 1;
    }
  }
}

const __FlashStringHelper * getPluginIDAndPrefix(char selection, pluginID_t& pluginID, bool& success)
{
  success = true;
  switch (tolower(selection))
  {
    case 'g': // gpio
    case 'l': // longpulse (gpio)
      pluginID  = PLUGIN_GPIO;
      return F("GPIO");
#ifdef USES_P009
    case 'm': // mcp & mcplongpulse
      pluginID  = PLUGIN_MCP;
      return F("MCP");
#endif
#ifdef USES_P019
    case 'p': // pcf & pcflongpulse
      pluginID  = PLUGIN_PCF;
      return F("PCF");
#endif
    default:
      break;
  }
  success = false;
  return F("Plugin not included in build");
}

struct range_pattern_helper_data {
  range_pattern_helper_data() {
    // Make sure the pointer is always set.
    logPrefix = F("GPIO");
  }


  const __FlashStringHelper *  logPrefix;
  uint32_t write = 0;
  uint32_t mask  = 0;

  uint8_t firstPin     = 0;
  uint8_t lastPin      = 0;
  uint8_t numBytes     = 0;
  uint8_t deltaStart   = 0;
  uint8_t numBits      = 0;
  uint8_t firstAddress = 0;
  uint8_t firstBank    = 0;
  uint8_t initVal      = 0;
  bool isMask       = false;
  bool valid        = false;
};


range_pattern_helper_data range_helper_shared(pluginID_t plugin, uint8_t pin1, uint8_t pin2)
{
  range_pattern_helper_data data;

  switch (plugin) {
    case PLUGIN_PCF:
      data.logPrefix = F("PCF");
      break;
    case PLUGIN_MCP:
      data.logPrefix = F("MCP");
      break;
  }

  if ((pin2 < pin1) ||
      !checkValidPortRange(plugin, pin1) ||
      !checkValidPortRange(plugin, pin2) ||
      ((pin2 - pin1 + 1) > 16)) {
    if (loglevelActiveFor(LOG_LEVEL_ERROR)) {
      addLogMove(LOG_LEVEL_ERROR, concat(data.logPrefix, F(": pin numbers out of range.")));
    }
    return data;
  }

  data.firstPin   = ((pin1 - 1) & 0xF8) + 1;
  data.lastPin    = ((pin2 - 1) & 0xF8) + 8;
  data.numBytes   = (data.lastPin - data.firstPin + 1) / 8;
  data.deltaStart = pin1 - data.firstPin;
  data.numBits    = pin2 - pin1 + 1;

  if (plugin == PLUGIN_MCP) {
    data.firstAddress = ((pin1 - 1) / 16) + 0x20;
    data.firstBank    = (((data.firstPin - 1) / 8) + 2) % 2;
    data.initVal      = 2 * data.firstAddress + data.firstBank;
  }

  data.valid = true;
  return data;
}

range_pattern_helper_data range_pattern_helper_shared(pluginID_t plugin, struct EventStruct *event, const char *Line, bool isWritePattern)
{
  range_pattern_helper_data data = range_helper_shared(plugin, event->Par1, event->Par2);

  if (!data.valid) {
    return data;
  }

  if (isWritePattern) {
    data.logPrefix = F("GPIOPattern");
  } else {
    data.logPrefix = F("GPIORange");
  }
  data.valid  = false;
  data.isMask = !parseString(Line, 5).isEmpty();

  if (data.isMask) {
    data.mask  = event->Par4 & ((1 << (data.numBytes * 8)) - 1);
    data.mask &= ((1 << data.numBits) - 1);
    data.mask  = data.mask << data.deltaStart;
  } else {
    data.mask = (1 << data.numBits) - 1;
    data.mask = data.mask << (data.deltaStart);
  }

  if (isWritePattern) {                                         // write pattern is present
    data.write  = event->Par3 & ((1 << (data.numBytes * 8)) - 1); // limit number of bytes
    data.write &= ((1 << data.numBits) - 1);                    // limit to number of bits
    data.write  = data.write << data.deltaStart;                // shift to start from starting pin
  } else {                                                      // write pattern not present
    if (event->Par3 == 0) {
      data.write = 0;
    } else if (event->Par3 == 1) {
      data.write = (1 << data.numBits) - 1;
      data.write = data.write << data.deltaStart;
    } else {
      if (loglevelActiveFor(LOG_LEVEL_ERROR)) {
        addLogMove(LOG_LEVEL_ERROR, concat(data.logPrefix,  F(": write value must be 0 or 1.")));
      }
      return data;
    }
  }


  data.valid = true;
  return data;
}

/******************************************************************************
** Par1=starting pin
** Par2=ending pin (must be higher of starting pin; and maximum 16 pin per command)
** Par3=write pattern: it's a write pattern. Write 0 or 1.
**                     Example: use decimal number 15 (in binary is 00001111) to set to 1 pin 1,2,3 and 4 and to set to 0 pins 5,6,7,8
**                     if number of bit lower than number of pins, then padded with 0;
**                     if number of bit higher than number of pins, then it's truncated.
** Par4=mask (optional): if not present assume to operate in all pins; if present is used as a mask (1=update, 0=do not update).
**            if number of bit lower than number of pins, then padded with 0;
**            if number of bit higher than number of pins, then it's truncated.
**
**  examples:
**  mcpgpioPattern,1,8,255
**     write pattern = '1101' that will be padded as: '0000001101'
**     mask not present, assume mask = '1111111111'
**  mcpgpioPattern,3,12,13
**     write pattern = '1101' that will be padded as: '0000001101'
**     mask not present, assume mask = '1111111111'
**  mcpgpioPattern,3,12,525
**     write pattern = 525 = '100001101'
**     mask not present, assume mask = '1111111111'
**  mcpgpioPattern,3,12,525,973
**     write pattern = 525 = '100001101'
**     mask = 973 = '1111001101'
**     write pattern after mask = '1000xx11x1' where x indicates that the pin will not be changed
******************************************************************************/
#ifdef USES_P009
const __FlashStringHelper * Command_GPIO_McpGPIOPattern(struct EventStruct *event, const char *Line)
{
  return mcpgpio_range_pattern_helper(event, Line, true) ? return_command_success() : return_command_failed();
}
#endif

#ifdef USES_P019
const __FlashStringHelper * Command_GPIO_PcfGPIOPattern(struct EventStruct *event, const char *Line)
{
  return pcfgpio_range_pattern_helper(event, Line, true) ? return_command_success() : return_command_failed();
}
#endif

/******************************************************************************
** Par1=starting pin
** Par2=ending pin (must be higher of starting pin; and maximum 16 pin per command)
** Par3=write value: if 0 (or 1) then assume 0 (or 1) for all the pins in the range;
** Par4=mask (optional): if not present assume to operate in all pins; if present is used as a mask (1=update, 0=do not update).
**            if number of bit lower than number of pins, then padded with 0;
**            if number of bit higher than number of pins, then it's truncated.
**
**  examples:
**  mcpgpioRange,1,8,1: set pins 1 to 8 to 1
**  mcpgpioRange,3,12,1: set pins 3 to 12 to 1
**  mcpgpioRange,5,17,0: set pins 5 to 17 to 0
**  mcpgpioRange,3,12,1,525
**     mask = '0100001101'
**     write pattern after mask = 'x1xxxx11x1' where x indicates that the pin will not be changed
**  mcpgpioRange,3,12,1,973
**     mask = 973 = '1111001101'
**     write pattern after mask = '1111xx11x1' where x indicates that the pin will not be changed
******************************************************************************/
#ifdef USES_P009
const __FlashStringHelper * Command_GPIO_McpGPIORange(struct EventStruct *event, const char *Line)
{
  return mcpgpio_range_pattern_helper(event, Line, false) ? return_command_success() : return_command_failed();
}
#endif

#ifdef USES_P019
const __FlashStringHelper * Command_GPIO_PcfGPIORange(struct EventStruct *event, const char *Line)
{
  return pcfgpio_range_pattern_helper(event, Line, false) ? return_command_success() : return_command_failed();
}
#endif

#ifdef USES_P009
bool mcpgpio_range_pattern_helper(struct EventStruct *event, const char *Line, bool isWritePattern)
{
  range_pattern_helper_data data = range_pattern_helper_shared(PLUGIN_MCP, event, Line, isWritePattern);

  if (!data.valid) {
    return false;
  }

  bool   onLine = false;
  for (uint8_t i = 0; i < data.numBytes; i++) {
    uint8_t readValue;
    const uint8_t    currentVal            = data.initVal + i;
    const uint8_t    currentAddress        = static_cast<int>(currentVal / 2);
    uint8_t          currentMask           = (data.mask  >> (8 * i)) & 0xFF;
    const uint8_t    currentInvertedMask   = 0xFF - currentMask;
    const uint8_t    currentWrite          = (data.write >> (8 * i)) & 0xFF;
    const uint8_t    currentGPIORegister   = ((currentVal % 2) == 0) ? MCP23017_GPIOA : MCP23017_GPIOB;
    const uint8_t    currentIOModeRegister = ((currentVal % 2) == 0) ? MCP23017_IODIRA : MCP23017_IODIRB;
    uint8_t    writeGPIOValue        = 0;

    if (GPIO_MCP_ReadRegister(currentAddress, currentIOModeRegister, &readValue)) {
      // set type to output only for the pins of the mask
      uint8_t writeModeValue = (readValue & currentInvertedMask);
      GPIO_MCP_WriteRegister(currentAddress, currentIOModeRegister, writeModeValue);
      GPIO_MCP_ReadRegister(currentAddress, currentGPIORegister, &readValue);

      // write to port
      writeGPIOValue = (readValue & currentInvertedMask) | (currentWrite & data.mask);
      GPIO_MCP_WriteRegister(currentAddress, currentGPIORegister, writeGPIOValue);

      onLine = true;
    } else {
      onLine = false;
    }

    const uint8_t   mode = (onLine) ? PIN_MODE_OUTPUT : PIN_MODE_OFFLINE;
    for (uint8_t j = 0; currentMask != 0 && j < 8; j++) {
      if (currentMask & 1) {  // only for the pins in the mask
        uint8_t currentPin    = data.firstPin + j + 8 * i;
        const uint32_t key = createKey(PLUGIN_MCP, currentPin);

        // state = onLine ? ((writeGPIOValue & uint8_t(pow(2,j))) >> j) : -1;
        const int8_t state = onLine ? ((writeGPIOValue & (1 << j)) >> j) : -1;

        createAndSetPortStatus_Mode_State(key, mode, state);
        String log;
        log += data.logPrefix;
        log += concat(F(": port#"), static_cast<int>(currentPin));
        log += concat(F(": set to "), static_cast<int>(state));
        addLog(LOG_LEVEL_INFO, log);
        SendStatusOnlyIfNeeded(event, SEARCH_PIN_STATE, key, log, 0);
      }
      currentMask >>= 1; // Shift the mask 1 position
    }
  }
  return onLine;
}
#endif

#ifdef USES_P019
uint8_t getPcfAddress(uint8_t pin)
{
  uint8_t retValue = static_cast<int>((pin - 1) / 8) + 0x20;

  if (retValue > 0x27) { retValue += 0x10; }
  return retValue;
}

bool pcfgpio_range_pattern_helper(struct EventStruct *event, const char *Line, bool isWritePattern)
{
  range_pattern_helper_data data = range_pattern_helper_shared(PLUGIN_PCF, event, Line, isWritePattern);

  if (!data.valid) {
    return false;
  }

  bool   onLine = false;
  String log;

  for (uint8_t i = 0; i < data.numBytes; i++) {
    uint8_t readValue;
    uint8_t    currentAddress = getPcfAddress(event->Par1 + 8 * i);

    uint8_t currentMask         = (data.mask  >> (8 * i)) & 0xFF;
    uint8_t currentInvertedMask = 0xFF - currentMask;
    uint8_t currentWrite        = (data.write >> (8 * i)) & 0xFF;
    uint8_t writeGPIOValue      = 255;

    onLine = GPIO_PCF_ReadAllPins(currentAddress, &readValue);

    if (onLine) { writeGPIOValue = (readValue & currentInvertedMask) | (currentWrite & data.mask); }

    uint8_t   mode = (onLine) ? PIN_MODE_OUTPUT : PIN_MODE_OFFLINE;
    int8_t state;

    for (uint8_t j = 0; j < 8; j++) {
      uint8_t currentPin    = data.firstPin + j + 8 * i;
      const uint32_t key = createKey(PLUGIN_PCF, currentPin);

      if (currentMask & 1) {  // only for the pins in the mask
        state = onLine ? ((writeGPIOValue & (1 << j)) >> j) : -1;

        createAndSetPortStatus_Mode_State(key, mode, state);
        log = concat(data.logPrefix, F(": port#")) + String(currentPin) + String(F(": set to ")) + String(state);
        addLog(LOG_LEVEL_INFO, log);
        SendStatusOnlyIfNeeded(event, SEARCH_PIN_STATE, key, log, 0);
      } else {
        // set to 1 the INPUT pins and the PIN that have not been initialized yet.
        if (!existPortStatus(key) ||
            (existPortStatus(key) &&
             ((globalMapPortStatus[key].mode == PIN_MODE_INPUT) || (globalMapPortStatus[key].mode == PIN_MODE_INPUT_PULLUP)))) {
          readValue |= (1 << j); // set port j = 1
        }
      }
      currentMask >>= 1; // Shift the mask 1 position
    }

    if (onLine) {
      writeGPIOValue = (readValue & currentInvertedMask) | (currentWrite & data.mask);

      // write to port
      GPIO_PCF_WriteAllPins(currentAddress, writeGPIOValue);
    }
  }
  return onLine;
}
#endif

bool setGPIOMode(uint8_t pin, uint8_t mode)
{
  if (!checkValidPortRange(PLUGIN_GPIO, pin)) {
    return false;
  }
  switch (mode) {
    case PIN_MODE_OUTPUT:
      pinMode(pin, OUTPUT);
      break;
    case PIN_MODE_INPUT_PULLUP:
      setInternalGPIOPullupMode(pin);
      break;
    case PIN_MODE_INPUT:
      pinMode(pin, INPUT);
      break;
  }
  return true;
}

#ifdef USES_P009
bool setMCPMode(uint8_t pin, uint8_t mode)
{
  if (checkValidPortRange(PLUGIN_MCP, pin)) {
    switch (mode) {
      case PIN_MODE_OUTPUT:
        setMCPOutputMode(pin);
        break;
      case PIN_MODE_INPUT_PULLUP:
        setMCPInputAndPullupMode(pin, true);
        break;
      case PIN_MODE_INPUT:
        setMCPInputAndPullupMode(pin, false);
        break;
    }
    return true;
  } else {
    return false;
  }
}
#endif

#ifdef USES_P019
bool setPCFMode(uint8_t pin, uint8_t mode)
{
  if (checkValidPortRange(PLUGIN_PCF, pin)) {
    switch (mode) {
      case PIN_MODE_OUTPUT:
        // do nothing
        break;
      case PIN_MODE_INPUT_PULLUP:
      case PIN_MODE_INPUT:
        setPCFInputMode(pin);
        break;
    }
    return true;
  } else {
    return false;
  }
}
#endif

/***********************************************
 * event->Par1: PIN to be set
 * event->Par2: MODE to be set:
 *             0 = OUTPUT
 *             1 = INPUT PULLUP or INPUT PULLDOWN (only for GPIO16)
 *             2 = INPUT
 **********************************************/
const __FlashStringHelper * Command_GPIO_Mode(struct EventStruct *event, const char *Line)
{
  if (gpio_mode_range_helper(event->Par1, event->Par2, event, Line)) {
    return return_command_success();
  }
  return return_command_failed();
}

const __FlashStringHelper * Command_GPIO_ModeRange(struct EventStruct *event, const char *Line)
{
  bool success = true;

  for (uint8_t i = event->Par1; i <= event->Par2; i++) {
    success &= gpio_mode_range_helper(i, event->Par3, event, Line);
  }
  return success ? return_command_success() : return_command_failed();
}

bool gpio_mode_range_helper(uint8_t pin, uint8_t pinMode, struct EventStruct *event, const char *Line)
{
  pluginID_t pluginID = INVALID_PLUGIN_ID;
  bool success = false;

  // Line[0]='g':gpio; ='p':pcfgpio; ='m':mcpgpio
  const __FlashStringHelper * logPrefix = getPluginIDAndPrefix(Line[0], pluginID, success);
  const __FlashStringHelper * logPostfix = F(""); // = new char;

  if (success && checkValidPortRange(pluginID, pin))
  {
    // int8_t state=0;
    uint8_t mode = 255;

    // bool setSuccess=false;

    switch (pinMode) {
      case 0:
        mode       = PIN_MODE_OUTPUT;
        logPostfix = F("OUTPUT");
        break;
      case 1:
        mode       = PIN_MODE_INPUT_PULLUP;
        logPostfix = F("INPUT PULLUP");
        break;
      case 2:
        mode       = PIN_MODE_INPUT;
        logPostfix = F("INPUT");
        break;
    }

    if (mode < 255) {
      switch (pluginID) {
        case PLUGIN_GPIO:
          /* setSuccess = */ setGPIOMode(pin, mode);
          break;
#ifdef USES_P019
        case PLUGIN_PCF:
          // set pin = 1 when INPUT
          /* setSuccess = */ setPCFMode(pin, mode);
          break;
#endif
#ifdef USES_P009
        case PLUGIN_MCP:
          /* setSuccess = */ setMCPMode(pin, mode);
          break;
#endif
        default:
          addLog(LOG_LEVEL_ERROR, F("Plugin not included in build"));
          return false;
      }

      const uint32_t key = createKey(pluginID, pin);

      if (globalMapPortStatus[key].mode != PIN_MODE_OFFLINE)
      {
        int8_t currentState;
        GPIO_Read(pluginID, pin, currentState);

        // state = currentState;

        if (currentState == -1) {
          mode = PIN_MODE_OFFLINE;

          // state = -1;
        }

        createAndSetPortStatus_Mode_State(key, mode, currentState);

        const String log = concat(logPrefix, F(" : port#")) + String(pin) + String(F(": MODE set to ")) + logPostfix + String(F(". Value = ")) +
                     String(currentState);
        addLog(LOG_LEVEL_INFO, log);
        SendStatusOnlyIfNeeded(event, SEARCH_PIN_STATE, key, log, 0);
        return true;
      } else {
        logErrorGpioOffline(logPrefix, pin);
        return false;
      }
    }
    logErrorModeOutOfRange(logPrefix, pin);
    return false;
  }
  logErrorGpioOutOfRange(logPrefix, pin, Line);
  return false;
}

bool getGPIOPinStateValues(String& str) {
  // parseString(string, 1) = device (gpio,mcpgpio,pcfgpio) that can be shortened to g, m or p
  // parseString(string, 2) = command (pinstate,pinrange)
  // parseString(string, 3) = gpio 1st number or a range separated by '-'
  bool   success = false;
  const __FlashStringHelper * logPrefix = F("");
  const String device     = parseString(str, 1);
  const String command    = parseString(str, 2);
  const String gpio_descr = parseString(str, 3);

  if ((command.length() >= 8) && command.equalsIgnoreCase(F("pinstate")) && (device.length() > 0)) {
    // returns pin value using syntax: [plugin#xxxxxxx#pinstate#x]
    int par1;
    const bool validArgument = validIntFromString(gpio_descr, par1);

    if (validArgument) {
      switch (device[0]) {
        case 'g':

          str       = digitalRead(par1);
          logPrefix = F("GPIO");
          success   = true;
          break;

#ifdef USES_P009
        case 'm':
          str       = GPIO_MCP_Read(par1);
          logPrefix = F("MCP");
          success   = true;
          break;
#endif

#ifdef USES_P019
        case 'p':
          str       = GPIO_PCF_Read(par1);
          logPrefix = F("PCF");
          success   = true;
          break;
#endif
        default:
          addLog(LOG_LEVEL_ERROR, F("Plugin not included in build"));
          return false;
      }
    }

    if (success) {
      #ifndef BUILD_NO_DEBUG
      addLog(LOG_LEVEL_DEBUG, String(logPrefix) + F(" PLUGIN PINSTATE pin =") + String(par1) + F("; value=") + str);
      #endif // ifndef BUILD_NO_DEBUG
    } else {
      addLog(LOG_LEVEL_ERROR, F(" PLUGIN PINSTATE. Syntax error. Pin parameter is not numeric"));
    }
  } else if ((command.length() >= 8) && command.equalsIgnoreCase(F("pinrange"))) {
    // returns pin value using syntax: [plugin#xxxxxxx#pinrange#x-y]
    int  par1, par2;
    bool successPar = false;
    int  dashpos    = gpio_descr.indexOf('-');

    if (dashpos != -1) {
      // Found an extra '-' in the 4th param, will split.
      successPar  = validIntFromString(gpio_descr.substring(dashpos + 1), par2);
      successPar &= validIntFromString(gpio_descr.substring(0, dashpos), par1);
    }

    if (successPar) {

      switch (device[0]) {
#ifdef USES_P009
        case 'm':
        {
          uint16_t tempValue = 0;
          logPrefix = F("MCP");
          success   = mcpgpio_plugin_range_helper(par1, par2, tempValue);
          str       = String(tempValue);
          break;
        }
#endif

#ifdef USES_P019
        case 'p':
        {
          uint16_t tempValue = 0;
          logPrefix = F("PCF");
          success   = pcfgpio_plugin_range_helper(par1, par2, tempValue);
          str       = String(tempValue);
          break;
        }
          #endif
        default:
          addLog(LOG_LEVEL_ERROR, F("PLUGIN PINSTATE. Plugin not included in build"));
          return false;

      }

      if (success) {
        #ifndef BUILD_NO_DEBUG
        addLogMove(LOG_LEVEL_DEBUG,
               concat(logPrefix, F(" PLUGIN RANGE pin start=")) + String(par1) + 
               concat(F("; pin end="), par2) + 
               concat(F("; value="), str));
        #endif // ifndef BUILD_NO_DEBUG
      } else {
        addLogMove(LOG_LEVEL_ERROR,
               concat(logPrefix,  F(" IS OFFLINE. PLUGIN RANGE pin start=")) + String(par1) + 
               concat(F("; pin end="), par2) +
               concat(F("; value="), str));
      }
    } else {
      addLog(LOG_LEVEL_ERROR, F(" PLUGIN PINRANGE. Syntax error. Pin parameters are not numeric."));
    }
  } else {
    addLog(LOG_LEVEL_ERROR, F("Syntax error. Invalid command. Valid commands are 'pinstate' and 'pinrange'."));
  }

  if (!success) {
    str = '0';
  }
  return success;
}

#ifdef USES_P009
bool mcpgpio_plugin_range_helper(uint8_t pin1, uint8_t pin2, uint16_t& result)
{
  const range_pattern_helper_data data = range_helper_shared(PLUGIN_MCP, pin1, pin2);

  if (!data.valid) {
    return false;
  }

  //  data.logPrefix += F("PluginRead");

  String log;
  bool success = false;
  uint32_t tempResult = 0;

  for (uint8_t i = 0; i < data.numBytes; i++) {
    uint8_t readValue                 = 0;
    const uint8_t currentVal          = data.initVal + i;
    const uint8_t currentAddress      = static_cast<int>(currentVal / 2);
    const uint8_t currentGPIORegister = ((currentVal % 2) == 0) ? MCP23017_GPIOA : MCP23017_GPIOB;

    const bool onLine = GPIO_MCP_ReadRegister(currentAddress, currentGPIORegister, &readValue);

    if (onLine) {
      success = true; // One valid address
      tempResult += (static_cast<uint32_t>(readValue) << (8 * i)); 
    }
  }

  tempResult  = tempResult >> data.deltaStart;
  tempResult &= ((1 << data.numBits) - 1);
  result      = uint16_t(tempResult);

  return success;
}
#endif

#ifdef USES_P019
bool pcfgpio_plugin_range_helper(uint8_t pin1, uint8_t pin2, uint16_t& result)
{
  const range_pattern_helper_data data = range_helper_shared(PLUGIN_PCF, pin1, pin2);

  if (!data.valid) {
    return false;
  }

  //  data.logPrefix += F("PluginRead");

  String log;

  bool success = false;
  uint32_t tempResult = 0;

  for (uint8_t i = 0; i < data.numBytes; i++) {
    uint8_t readValue         = 0;
    const uint8_t currentAddress = getPcfAddress(pin1 + 8 * i);

    const bool onLine = GPIO_PCF_ReadAllPins(currentAddress, &readValue);

    if (onLine) { 
      success = true; // One valid address
      tempResult += (static_cast<uint32_t>(readValue) << (8 * i)); 
    }
  }

  tempResult  = tempResult >> data.deltaStart;
  tempResult &= ((1 << data.numBits) - 1);
  result      = uint16_t(tempResult);

  return success;
}
#endif
#include "../Commands/UPD.h"


#include "../../ESPEasy_common.h"

#include "../Commands/Common.h"
#include "../ESPEasyCore/ESPEasyNetwork.h"
#include "../Globals/NetworkState.h"
#include "../Globals/Settings.h"
#include "../Helpers/Misc.h"
#include "../Helpers/Network.h"
#include "../Helpers/Networking.h"
#include "../Helpers/StringConverter.h"
#include "../Helpers/StringParser.h"

String Command_UDP_Port(struct EventStruct *event, const char *Line)
{
  return Command_GetORSetBool(event, F("UDPPort:"),
                              Line,
                              (bool *)&Settings.UDPPort,
                              1);
}

#if FEATURE_ESPEASY_P2P

const __FlashStringHelper * Command_UDP_Test(struct EventStruct *event, const char *Line)
{
  for (uint8_t x = 0; x < event->Par2; x++)
  {
    String eventName = "Test ";
    eventName += x;
    SendUDPCommand(event->Par1, eventName.c_str(), eventName.length());
  }
  return return_command_success();
}


const __FlashStringHelper * Command_UPD_SendTo(struct EventStruct *event, const char *Line)
{
  int destUnit = parseCommandArgumentInt(Line, 1);
  if ((destUnit > 0) && (destUnit < 255))
  {
    String eventName = tolerantParseStringKeepCase(Line, 3);
    SendUDPCommand(destUnit, eventName.c_str(), eventName.length());
  }
  return return_command_success();
}
#endif //FEATURE_ESPEASY_P2P

const __FlashStringHelper * Command_UDP_SendToUPD(struct EventStruct *event, const char *Line)
{
  if (NetworkConnected()) {
    String ip      = parseString(Line, 2);
    int port    = parseCommandArgumentInt(Line, 2);

    if (port < 0 || port > 65535) return return_command_failed();
    // FIXME TD-er: This command is not using the tolerance setting
    // tolerantParseStringKeepCase(Line, 4);
    String message = parseStringToEndKeepCase(Line, 4);
    IPAddress UDP_IP;

    if (UDP_IP.fromString(ip)) {
      FeedSW_watchdog();
      portUDP.beginPacket(UDP_IP, port);
      #if defined(ESP8266)
      portUDP.write(message.c_str(),            message.length());
      #endif // if defined(ESP8266)
      #if defined(ESP32)
      portUDP.write(reinterpret_cast<const uint8_t *>(message.c_str()), message.length());
      #endif // if defined(ESP32)
      portUDP.endPacket();
      FeedSW_watchdog();
      delay(0);
    }
    return return_command_success();
  }
  return return_not_connected();
}

#include "../Commands/Servo.h"

#include "../Commands/Common.h"
#if FEATURE_SERVO

#include "../DataStructs/EventStructCommandWrapper.h"
#include "../DataStructs/PinMode.h"
#include "../DataStructs/PortStatusStruct.h"
#include "../ESPEasyCore/Controller.h"
#include "../ESPEasyCore/ESPEasyGPIO.h"
#include "../ESPEasyCore/ESPEasy_Log.h"
#include "../Globals/GlobalMapPortStatus.h"
#include "../Helpers/Hardware.h"
#include "../Helpers/PortStatus.h"
#include "../Helpers/StringConverter.h"

// Needed also here for PlatformIO's library finder as the .h file 
// is in a directory which is excluded in the src_filter
# include <Servo.h>
ServoPinMap_t ServoPinMap;
#endif // if FEATURE_SERVO

const __FlashStringHelper * Command_Servo(struct EventStruct *event, const char *Line)
{
  #if FEATURE_SERVO

  // GPIO number is stored inside event->Par2 instead of event->Par1 as in all the other commands
  // So needs to reload the tempPortStruct.

  // FIXME TD-er: For now only fixed to "P001" even when it is for internal GPIO pins
  pluginID_t pluginID = PLUGIN_GPIO;

  // Par1: Servo ID (obsolete/unused since 2020/11/22)
  // Par2: GPIO pin
  // Par3: angle 0...180 degree
  if (checkValidPortRange(pluginID, event->Par2)) {
    portStatusStruct tempStatus;
    const uint32_t   key = createKey(pluginID, event->Par2); // WARNING: 'servo' uses Par2 instead of Par1
    // WARNING: operator [] creates an entry in the map if key does not exist
    // So the next command should be part of each command:
    tempStatus = globalMapPortStatus[key];

    String log = concat(F("Servo : GPIO "), event->Par2);

    // SPECIAL CASE TO ALLOW SERVO TO BE DETATTCHED AND SAVE POWER.
    if (event->Par3 >= 9000) {
      auto it = ServoPinMap.find(event->Par2);

      if (it != ServoPinMap.end()) {
        it->second.detach();
        # ifdef ESP32
          detachLedChannel(event->Par2);
        # endif // ifdef ESP32
        ServoPinMap.erase(it);
      }

      // Set parameters to make sure the port status will be removed.
      tempStatus.task    = 0;
      tempStatus.monitor = 0;
      tempStatus.command = 0;
      savePortStatus(key, tempStatus);
      addLog(LOG_LEVEL_INFO, concat(log, F(" Servo detached")));
      return return_command_success();

    }
    # ifdef ESP32
      // Must keep track of used channels or else cause conflicts with PWM
      int8_t ledChannel = attachLedChannel(event->Par2);
      ServoPinMap[event->Par2].attach(event->Par2, ledChannel);
    # else // ifdef ESP32
      ServoPinMap[event->Par2].attach(event->Par2);
    # endif // ifdef ESP32
    ServoPinMap[event->Par2].write(event->Par3);

    tempStatus.command   = 1; // set to 1 in order to display the status in the PinStatus page
    tempStatus.state     = 1;
    tempStatus.output    = 1;
    tempStatus.dutyCycle = event->Par3;

    // setPinState(PLUGIN_ID_001, event->Par2, PIN_MODE_SERVO, event->Par3);
    tempStatus.mode = PIN_MODE_SERVO;
    savePortStatus(key, tempStatus);
    log += F(" Servo set to ");
    log += event->Par3;
    addLog(LOG_LEVEL_INFO, log);
    SendStatusOnlyIfNeeded(event, SEARCH_PIN_STATE, key, log, 0);

    // SendStatus(event, getPinStateJSON(SEARCH_PIN_STATE, PLUGIN_ID_001, event->Par2, log, 0));
    return return_command_success();
  }
  #else // if FEATURE_SERVO
  addLog(LOG_LEVEL_ERROR, F("SERVO : command not included in build"));
  #endif // FEATURE_SERVO
  return return_command_failed();
}

#include "../Commands/Networks.h"

#include "../../ESPEasy_common.h"
#include "../Commands/Common.h"
#include "../ESPEasyCore/ESPEasyNetwork.h"
#include "../ESPEasyCore/ESPEasyEth.h"
#include "../Globals/NetworkState.h"
#include "../Globals/Settings.h"
#include "../Helpers/StringConverter.h"
#include "../WebServer/AccessControl.h"


#if FEATURE_ETHERNET
#include <ETH.h>
#endif

String Command_AccessInfo_Ls(struct EventStruct *event, const char* Line)
{
  return return_result(event, concat(F("Allowed IP range : "), describeAllowedIPrange()));
}

String Command_AccessInfo_Clear (struct EventStruct *event, const char* Line)
{
  clearAccessBlock();
  return Command_AccessInfo_Ls(event, Line);
}

String Command_DNS (struct EventStruct *event, const char* Line)
{
  return Command_GetORSetIP(event, F("DNS:"), Line, Settings.DNS, NetworkDnsIP(0), 1);
}

String Command_Gateway (struct EventStruct *event, const char* Line)
{
  return Command_GetORSetIP(event, F("Gateway:"), Line, Settings.Gateway, NetworkGatewayIP(),1);
}

String Command_IP (struct EventStruct *event, const char* Line)
{
  return Command_GetORSetIP(event, F("IP:"), Line, Settings.IP, NetworkLocalIP(),1);
}

String Command_Subnet (struct EventStruct *event, const char* Line)
{
  return Command_GetORSetIP(event, F("Subnet:"), Line, Settings.Subnet, NetworkSubnetMask(), 1);
}

#if FEATURE_ETHERNET
String Command_ETH_Phy_Addr (struct EventStruct *event, const char* Line)
{
  return Command_GetORSetInt8_t(event, F("ETH_Phy_Addr:"), Line, reinterpret_cast<int8_t*>(&Settings.ETH_Phy_Addr),1);
}

String Command_ETH_Pin_mdc (struct EventStruct *event, const char* Line)
{
  return Command_GetORSetInt8_t(event, F("ETH_Pin_mdc:"), Line, reinterpret_cast<int8_t*>(&Settings.ETH_Pin_mdc),1);
}

String Command_ETH_Pin_mdio (struct EventStruct *event, const char* Line)
{
  return Command_GetORSetInt8_t(event, F("ETH_Pin_mdio:"), Line, reinterpret_cast<int8_t*>(&Settings.ETH_Pin_mdio),1);
}

String Command_ETH_Pin_power (struct EventStruct *event, const char* Line)
{
  return Command_GetORSetInt8_t(event, F("ETH_Pin_power:"), Line, reinterpret_cast<int8_t*>(&Settings.ETH_Pin_power),1);
}

String Command_ETH_Phy_Type (struct EventStruct *event, const char* Line)
{
  return Command_GetORSetInt8_t(event, F("ETH_Phy_Type:"), Line, reinterpret_cast<int8_t*>(&Settings.ETH_Phy_Type),1);
}

String Command_ETH_Clock_Mode (struct EventStruct *event, const char* Line)
{
  return Command_GetORSetETH(event, 
                             F("ETH_Clock_Mode:"), 
                             toString(Settings.ETH_Clock_Mode),
                             Line, 
                             reinterpret_cast<uint8_t*>(&Settings.ETH_Clock_Mode),
                             1);
}

String Command_ETH_IP (struct EventStruct *event, const char* Line)
{
  return Command_GetORSetIP(event, F("ETH_IP:"), Line, Settings.ETH_IP,ETH.localIP(),1);
}

String Command_ETH_Gateway (struct EventStruct *event, const char* Line)
{
  return Command_GetORSetIP(event, F("ETH_Gateway:"), Line, Settings.ETH_Gateway,ETH.gatewayIP(),1);
}

String Command_ETH_Subnet (struct EventStruct *event, const char* Line)
{
  return Command_GetORSetIP(event, F("ETH_Subnet:"), Line, Settings.ETH_Subnet,ETH.subnetMask(),1);
}

String Command_ETH_DNS (struct EventStruct *event, const char* Line)
{
  return Command_GetORSetIP(event, F("ETH_DNS:"), Line, Settings.ETH_DNS,ETH.dnsIP(),1);
}

String Command_ETH_Wifi_Mode (struct EventStruct *event, const char* Line)
{
  const NetworkMedium_t orig_medium = Settings.NetworkMedium;
  const String result = Command_GetORSetETH(event, 
                             F("NetworkMedium:"), 
                             toString(active_network_medium),
                             Line, 
                             reinterpret_cast<uint8_t*>(&Settings.NetworkMedium), 
                             1);
  if (orig_medium != Settings.NetworkMedium) {
    if (!isValid(Settings.NetworkMedium)) {
      Settings.NetworkMedium = orig_medium;
      return return_command_failed();
    }
    setNetworkMedium(Settings.NetworkMedium);
  }
  
  return result;
}

String Command_ETH_Disconnect (struct EventStruct *event, const char* Line)
{

  ethPower(0);
  delay(400);
//  ethPower(1);
  setNetworkMedium(NetworkMedium_t::Ethernet);
  ETHConnectRelaxed();

  return return_command_success_str();
}

#endif // if FEATURE_ETHERNET

#include "../Commands/Notifications.h"

#if FEATURE_NOTIFIER

#include "../Commands/Common.h"

#include "../../ESPEasy_common.h"
#include "../DataTypes/ESPEasy_plugin_functions.h"
#include "../Globals/ESPEasy_Scheduler.h"
#include "../Globals/Settings.h"
#include "../Globals/NPlugins.h"
#include "../Helpers/StringConverter.h"


const __FlashStringHelper * Command_Notifications_Notify(struct EventStruct *event, const char* Line)
{
	String message;
	GetArgv(Line, message, 3);

	if (event->Par1 > 0) {
		int index = event->Par1 - 1;
		if (Settings.NotificationEnabled[index] && Settings.Notification[index] != 0) {
			nprotocolIndex_t NotificationProtocolIndex = getNProtocolIndex(Settings.Notification[index]);
			if (validNProtocolIndex(NotificationProtocolIndex )) {
				struct EventStruct TempEvent(event->TaskIndex);
				// TempEvent.NotificationProtocolIndex = NotificationProtocolIndex;
				TempEvent.NotificationIndex = index;
				TempEvent.String1 = message;
				Scheduler.schedule_notification_event_timer(NotificationProtocolIndex, NPlugin::Function::NPLUGIN_NOTIFY, std::move(TempEvent));
			}
		}
	}
	return return_command_success();
}

#endif
#include "../Commands/Tasks.h"


#include "../../ESPEasy_common.h"


#include "../Commands/Common.h"

#include "../ESPEasyCore/Controller.h"
#include "../ESPEasyCore/Serial.h"

#include "../Globals/RuntimeData.h"
#include "../Globals/Settings.h"

#include "../Helpers/Misc.h"
#include "../Helpers/Rules_calculate.h"
#include "../Helpers/StringConverter.h"
#include "../Helpers/StringParser.h"

//      taskIndex = (event->Par1 - 1);   Par1 is here for 1 ... TASKS_MAX
//	varNr = event->Par2 - 1;
bool validTaskVars(struct EventStruct *event, taskIndex_t& taskIndex, unsigned int& varNr)
{
  if (event == nullptr) { return false; }

  if (event->Par1 <= 0) { return false; }
  taskIndex_t tmp_taskIndex = static_cast<taskIndex_t>(event->Par1 - 1);

  varNr = 0;

  if (event->Par2 > 0) {
    varNr = event->Par2 - 1;
  }

  if (!validTaskIndex(tmp_taskIndex)) { return false; }

  if (varNr >= VARS_PER_TASK) { return false; }

  taskIndex = tmp_taskIndex;

  return true;
}

/**
 * parse TaskName/TaskValue when not numeric for task name and value name and validate values
 */
bool validateAndParseTaskValueArguments(struct EventStruct * event, const char *Line, taskIndex_t &taskIndex, unsigned int &varNr)
{
  if (!validTaskVars(event, taskIndex, varNr) || (event->Par2 <= 0 || event->Par2 >= VARS_PER_TASK))  // Extra check required because of shortcutting in validTaskVars()
  { 
    String taskName;
    taskIndex_t tmpTaskIndex = taskIndex;
    if ((event->Par1 <= 0 || event->Par1 >= INVALID_TASK_INDEX) && GetArgv(Line, taskName, 2)) {
      tmpTaskIndex = findTaskIndexByName(taskName, true);
      if (tmpTaskIndex != INVALID_TASK_INDEX) {
        event->Par1 = tmpTaskIndex + 1;
      }
    }
    String valueName;
    if ((event->Par2 <= 0 || event->Par2 >= VARS_PER_TASK) && event->Par1 - 1 != INVALID_TASK_INDEX && GetArgv(Line, valueName, 3))
    {
      uint8_t tmpVarNr = findDeviceValueIndexByName(valueName, event->Par1 - 1);
      if (tmpVarNr != VARS_PER_TASK) {
        event->Par2 = tmpVarNr + 1;
      }
    }
    if (!validTaskVars(event, taskIndex, varNr)) return false; 
  }

  return true;
}

const __FlashStringHelper * taskValueSet(struct EventStruct *event, const char *Line, taskIndex_t& taskIndex, bool& success)
{
  String TmpStr1;
  unsigned int varNr;

  if (!validateAndParseTaskValueArguments(event, Line, taskIndex, varNr)) {
    success = false;
    return F("INVALID_PARAMETERS");
  }
  if (!Settings.AllowTaskValueSetAllPlugins() && getPluginID_from_TaskIndex(taskIndex) != 33) { // PluginID 33 = Dummy Device
    success = false;
    return F("NOT_A_DUMMY_TASK");
  }
  if (!Settings.TaskDeviceEnabled[taskIndex]) {
    success = false;
    return F("TASK_NOT_ENABLED");
  }

  unsigned int uservarIndex = (VARS_PER_TASK * taskIndex) + varNr;

  if (GetArgv(Line, TmpStr1, 4)) {
    // Perform calculation with float result.
    double result = 0;

    if (isError(Calculate(TmpStr1, result))) {
      success = false;
      return F("CALCULATION_ERROR");
    }
    UserVar[uservarIndex] = result;
  } else  {
    // TODO: Get Task description and var name
    serialPrintln(toString(UserVar[uservarIndex]));
  }
  success = true;
  return return_command_success();
}

const __FlashStringHelper * Command_Task_Clear(struct EventStruct *event, const char *Line)
{
  taskIndex_t  taskIndex;
  unsigned int varNr;

  if (!validateAndParseTaskValueArguments(event, Line, taskIndex, varNr)) {
    return F("INVALID_PARAMETERS"); 
  }

  taskClear(taskIndex, true);
  return return_command_success();
}

const __FlashStringHelper * Command_Task_ClearAll(struct EventStruct *event, const char *Line)
{
  for (taskIndex_t t = 0; t < TASKS_MAX; t++) {
    taskClear(t, false);
  }
  return return_command_success();
}

const __FlashStringHelper * Command_Task_EnableDisable(struct EventStruct *event, bool enable, const char *Line)
{
  taskIndex_t  taskIndex;
  unsigned int varNr;

  if (validateAndParseTaskValueArguments(event, Line, taskIndex, varNr)) {
    // This is a command so no guarantee the taskIndex is correct in the event
    event->setTaskIndex(taskIndex);

    if (setTaskEnableStatus(event, enable)) {
      return return_command_success();
    }
    return return_command_failed();
  }
  return F("INVALID_PARAMETERS");
}

const __FlashStringHelper * Command_Task_Disable(struct EventStruct *event, const char *Line)
{
  return Command_Task_EnableDisable(event, false, Line);
}

const __FlashStringHelper * Command_Task_Enable(struct EventStruct *event, const char *Line)
{
  return Command_Task_EnableDisable(event, true, Line);
}

const __FlashStringHelper * Command_Task_ValueSet(struct EventStruct *event, const char *Line)
{
  taskIndex_t taskIndex;
  bool success;
  return taskValueSet(event, Line, taskIndex, success);
}

const __FlashStringHelper * Command_Task_ValueToggle(struct EventStruct *event, const char *Line)
{
  taskIndex_t  taskIndex;
  unsigned int varNr;

  if (!validateAndParseTaskValueArguments(event, Line, taskIndex, varNr)) {
    return F("INVALID_PARAMETERS");
  }
  if (!Settings.TaskDeviceEnabled[taskIndex]) {
    return F("TASK_NOT_ENABLED");
  }

  unsigned int uservarIndex = (VARS_PER_TASK * taskIndex) + varNr;
  const int    result       = lround(UserVar[uservarIndex]);

  if ((result == 0) || (result == 1)) {
    UserVar[uservarIndex] = (result == 0) ? 1.0f : 0.0f;
  }
  return return_command_success();
}

const __FlashStringHelper * Command_Task_ValueSetAndRun(struct EventStruct *event, const char *Line)
{
  taskIndex_t taskIndex;
  bool success;
  const __FlashStringHelper * returnvalue = taskValueSet(event, Line, taskIndex, success);
  if (success)
  {
    SensorSendTask(taskIndex);
    return return_command_success();
  }
  return returnvalue;
}

const __FlashStringHelper * Command_Task_Run(struct EventStruct *event, const char *Line)
{
  taskIndex_t  taskIndex;
  unsigned int varNr;

  if (!validateAndParseTaskValueArguments(event, Line, taskIndex, varNr)) {
    return F("INVALID_PARAMETERS");
  }
  if (!Settings.TaskDeviceEnabled[taskIndex]) {
    return F("TASK_NOT_ENABLED");
  }

  SensorSendTask(taskIndex);
  return return_command_success();
}

const __FlashStringHelper * Command_Task_RemoteConfig(struct EventStruct *event, const char *Line)
{
  struct EventStruct TempEvent(event->TaskIndex);
  String request = Line;

  // FIXME TD-er: Should we call ExecuteCommand here? The command is not parsed like any other call.
  remoteConfig(&TempEvent, request);
  return return_command_success();
}

#include "../Commands/RTC.h"

#include "../../ESPEasy_common.h"


#include "../Commands/Common.h"

#include "../DataStructs/RTCStruct.h"

#include "../Globals/RTC.h"

#include "../Helpers/ESPEasyRTC.h"


const __FlashStringHelper * Command_RTC_Clear(struct EventStruct *event, const char* Line)
{
	initRTC();
	return return_command_success();
}

const __FlashStringHelper * Command_RTC_resetFlashWriteCounter(struct EventStruct *event, const char* Line)
{
	RTC.flashDayCounter = 0;
	return return_command_success();
}

#include "../Commands/System.h"

#include "../../ESPEasy_common.h"


#include "../Commands/Common.h"

#include "../Globals/Settings.h"

#include "../Helpers/DeepSleep.h"
#include "../Helpers/Misc.h"
#include "../Helpers/Scheduler.h"

const __FlashStringHelper * Command_System_NoSleep(struct EventStruct *event, const char* Line)
{
	if (event->Par1 > 0)
		Settings.deepSleep_wakeTime = event->Par1; // set deep Sleep awake time
	else Settings.deepSleep_wakeTime = 0;
	return return_command_success();
}

const __FlashStringHelper * Command_System_deepSleep(struct EventStruct *event, const char* Line)
{
	if (event->Par1 >= 0) {
		deepSleepStart(event->Par1); // call the second part of the function to avoid check and enable one-shot operation
	}
	return return_command_success();
}

const __FlashStringHelper * Command_System_Reboot(struct EventStruct *event, const char* Line)
{
	pinMode(0, INPUT);
	pinMode(2, INPUT);
	pinMode(15, INPUT);
	reboot(ESPEasy_Scheduler::IntendedRebootReason_e::CommandReboot);
	return return_command_success();
}


#include "../Commands/HTTP.h"

#include "../../ESPEasy_common.h"

#include "../Commands/Common.h"

#include "../DataStructs/ControllerSettingsStruct.h"
#include "../DataStructs/SettingsStruct.h"

#include "../ESPEasyCore/ESPEasy_Log.h"
#include "../ESPEasyCore/ESPEasyNetwork.h"

#include "../Globals/Settings.h"

#include "../Helpers/_CPlugin_Helper.h"
#include "../Helpers/Misc.h"
#include "../Helpers/Networking.h"
#include "../Helpers/StringParser.h"


#if FEATURE_SEND_TO_HTTP
const __FlashStringHelper* Command_HTTP_SendToHTTP(struct EventStruct *event, const char *Line)
{
  if (NetworkConnected()) {
    String   user, pass, host, file, path;
    uint16_t port;

    const String arg1 = parseStringKeepCase(Line, 2);

    if (arg1.indexOf(F("://")) != -1) {
      // Full url given
      path = splitURL(arg1, user, pass, host, port, file);
    } else {
      // Command arguments are split into: host, port, url
      if (!splitUserPass_HostPortString(
            arg1,
            user,
            pass,
            host,
            port))
      {
        return return_command_failed();
      }

      const int port_arg = event->Par2;

      if ((port_arg > 0) && (port_arg < 65536)) {
        port = port_arg;
      } else {
        if (loglevelActiveFor(LOG_LEVEL_ERROR)) {
          String log = F("SendToHTTP: Invalid port argument: ");
          log += port_arg;
          log += F(" will use: ");
          log += port;
          addLogMove(LOG_LEVEL_ERROR, log);
        }
      }

      // FIXME TD-er: This is not using the tolerant settings option.
      // String path = tolerantParseStringKeepCase(Line, 4);
      path = parseStringToEndKeepCase(Line, 4);
    }
#ifndef BUILD_NO_DEBUG

    if (loglevelActiveFor(LOG_LEVEL_DEBUG)) {
      String log = F("SendToHTTP: Host: ");
      log += host;
      log += F(" port: ");
      log += port;
      log += F(" path: ");
      log += path;
      addLogMove(LOG_LEVEL_DEBUG, log);
    }
#endif // ifndef BUILD_NO_DEBUG

    // Some servers don't give an ack.
    // For these it is adviced to uncheck to wait for an acknowledgement.
    // However the default timeout of 4000 msec is then way too long
    const int timeout = Settings.SendToHttp_ack() 
       ? CONTROLLER_CLIENTTIMEOUT_MAX : 1000;
    // FIXME TD-er: Make sendToHttp timeout a setting.       

    int httpCode = -1;
    send_via_http(
      F("SendToHTTP"),
      timeout,
      user,
      pass,
      host,
      port,
      path,
      F("GET"),
      EMPTY_STRING, // header
      EMPTY_STRING, // poststr
      httpCode,
      Settings.SendToHttp_ack());

    if ((httpCode >= 100) && (httpCode < 300)) {
      return return_command_success();
    }
  } else {
    addLog(LOG_LEVEL_ERROR, F("SendToHTTP Not connected to network"));
  }
  return return_command_failed();
}
#endif // FEATURE_SEND_TO_HTTP

#include "../Commands/Rules.h"


#include "../../ESPEasy_common.h"


#include "../Commands/Common.h"

#include "../DataTypes/EventValueSource.h"

#include "../ESPEasyCore/Controller.h"
#include "../ESPEasyCore/ESPEasyRules.h"

#include "../Globals/EventQueue.h"
#include "../Globals/RuntimeData.h"
#include "../Globals/Settings.h"

#include "../Helpers/Misc.h"
#include "../Helpers/Rules_calculate.h"
#include "../Helpers/StringConverter.h"

const __FlashStringHelper * Command_Rules_Execute(struct EventStruct *event, const char *Line)
{
  String filename;

  if (GetArgv(Line, filename, 2)) {
    String event;
    rulesProcessingFile(filename, event);
  }
  return return_command_success();
}

String Command_Rules_UseRules(struct EventStruct *event, const char *Line)
{
  return Command_GetORSetBool(event, F("Rules:"),
                              Line,
                              (bool *)&Settings.UseRules,
                              1);
}

const __FlashStringHelper * Command_Rules_Async_Events(struct EventStruct *event, const char *Line)
{
  if (Settings.UseRules) {
    String eventName = parseStringToEndKeepCase(Line, 2);

    eventName.replace('$', '#');
    eventQueue.addMove(std::move(eventName));
  }
  return return_command_success();
}

const __FlashStringHelper * Command_Rules_Events(struct EventStruct *event, const char *Line)
{
  if (Settings.UseRules) {
    const bool executeImmediately =
      SourceNeedsStatusUpdate(event->Source) ||
      event->Source == EventValueSource::Enum::VALUE_SOURCE_RULES ||
      event->Source == EventValueSource::Enum::VALUE_SOURCE_RULES_RESTRICTED;

    String eventName = parseStringToEndKeepCase(Line, 2);

    eventName.replace('$', '#');
    if (executeImmediately) {
      rulesProcessing(eventName); // TD-er: Process right now
    } else {
      eventQueue.addMove(std::move(eventName));
    }
  }
  return return_command_success();
}

const __FlashStringHelper * Command_Rules_Let(struct EventStruct *event, const char *Line)
{
  String TmpStr1;

  if (GetArgv(Line, TmpStr1, 3)) {
    if (event->Par1 >= 0) {
      double result = 0.0;

      if (!isError(Calculate(TmpStr1, result))) {
        setCustomFloatVar(event->Par1, result);
        return return_command_success();
      }
    }
  }
  return return_command_failed();
}

#include "../Commands/Controller.h"


#include "../../ESPEasy_common.h"


#include "../Commands/Common.h"

#include "../DataStructs/ESPEasy_EventStruct.h"

#include "../DataTypes/ControllerIndex.h"

#include "../Globals/CPlugins.h"

#include "../Helpers/Misc.h"

//      controllerIndex = (event->Par1 - 1);   Par1 is here for 1 ... CONTROLLER_MAX
bool validControllerVar(struct EventStruct *event, controllerIndex_t& controllerIndex)
{
  if (event->Par1 <= 0) { return false; }
  controllerIndex = static_cast<controllerIndex_t>(event->Par1 - 1);
  return validControllerIndex(controllerIndex);
}

const __FlashStringHelper * Command_Controller_Disable(struct EventStruct *event, const char *Line)
{
  controllerIndex_t controllerIndex;

  if (validControllerVar(event, controllerIndex) && setControllerEnableStatus(controllerIndex, false)) {
    return return_command_success();
  }
  return return_command_failed();
}

const __FlashStringHelper * Command_Controller_Enable(struct EventStruct *event, const char *Line)
{
  controllerIndex_t controllerIndex;

  if (validControllerVar(event, controllerIndex) && setControllerEnableStatus(controllerIndex, true)) {
    return return_command_success();
  }
  return return_command_failed();
}

#include "../Commands/Diagnostic.h"


#include "../Commands/Common.h"

#include "../DataStructs/PortStatusStruct.h"

#include "../DataTypes/SettingsType.h"

#include "../ESPEasyCore/ESPEasy_backgroundtasks.h"
#include "../ESPEasyCore/ESPEasy_Log.h"
#include "../ESPEasyCore/Serial.h"

#include "../Globals/Device.h"
#include "../Globals/ExtraTaskSettings.h"
#include "../Globals/GlobalMapPortStatus.h"
#include "../Globals/SecuritySettings.h"
#include "../Globals/Settings.h"
#include "../Globals/Statistics.h"

#include "../Helpers/Convert.h"
#include "../Helpers/ESPEasy_Storage.h"
#include "../Helpers/ESPEasy_time_calc.h"
#include "../Helpers/Misc.h"
#include "../Helpers/PortStatus.h"
#include "../Helpers/StringConverter.h"
#include "../Helpers/StringParser.h"

#include <map>
#include <stdint.h>


#ifndef BUILD_MINIMAL_OTA
bool showSettingsFileLayout = false;
#endif // ifndef BUILD_MINIMAL_OTA

#ifndef BUILD_NO_DIAGNOSTIC_COMMANDS
String Command_Lowmem(struct EventStruct *event, const char *Line)
{
  String result;

  result += lowestRAM;
  result += F(" : ");
  result += lowestRAMfunction;
  return return_result(event, result);
}

const __FlashStringHelper * Command_Malloc(struct EventStruct *event, const char *Line)
{
  char *ramtest;
  int size = parseCommandArgumentInt(Line, 1);

  ramtest = (char *)malloc(size);

  if (ramtest == nullptr) { return return_command_failed(); }
  free(ramtest);
  return return_command_success();
}

String Command_SysLoad(struct EventStruct *event, const char *Line)
{
  String result = toString(getCPUload(), 2);

  result += F("% (LC=");
  result += getLoopCountPerSec();
  result += ')';
  return return_result(event, result);
}

const __FlashStringHelper * Command_SerialFloat(struct EventStruct *event, const char *Line)
{
  pinMode(1, INPUT);
  pinMode(3, INPUT);
  delay(60000);
  return return_command_success();
}

const __FlashStringHelper * Command_MemInfo(struct EventStruct *event, const char *Line)
{
  serialPrint(F("SecurityStruct         | "));
  serialPrintln(String(sizeof(SecuritySettings)));
  serialPrint(F("SettingsStruct         | "));
  serialPrintln(String(sizeof(Settings)));
  serialPrint(F("ExtraTaskSettingsStruct| "));
  serialPrintln(String(sizeof(ExtraTaskSettings)));
  serialPrint(F("DeviceStruct           | "));
  serialPrintln(String(Device.size()));
  return return_see_serial(event);
}

const __FlashStringHelper * Command_MemInfo_detail(struct EventStruct *event, const char *Line)
{
#ifndef BUILD_MINIMAL_OTA
  showSettingsFileLayout = true;
  Command_MemInfo(event, Line);

  for (int st = 0; st < static_cast<int>(SettingsType::Enum::SettingsType_MAX); ++st) {
    SettingsType::SettingsType::Enum settingsType = static_cast<SettingsType::Enum>(st);
    int max_index, offset, max_size;
    int struct_size = 0;
    serialPrintln();
    serialPrint(SettingsType::getSettingsTypeString(settingsType));
    serialPrintln(F(" | start | end | max_size | struct_size"));
    serialPrintln(F("--- | --- | --- | --- | ---"));
    SettingsType::getSettingsParameters(settingsType, 0, max_index, offset, max_size, struct_size);

    for (int i = 0; i < max_index; ++i) {
      SettingsType::getSettingsParameters(settingsType, i, offset, max_size);
      serialPrint(String(i));
      serialPrint("|");
      serialPrint(String(offset));
      serialPrint("|");
      serialPrint(String(offset + max_size - 1));
      serialPrint("|");
      serialPrint(String(max_size));
      serialPrint("|");
      serialPrintln(String(struct_size));
    }
  }
  return return_see_serial(event);
  #else
  return return_command_failed();
  #endif // ifndef BUILD_MINIMAL_OTA
}

const __FlashStringHelper * Command_Background(struct EventStruct *event, const char *Line)
{
  unsigned long timer = millis() + parseCommandArgumentInt(Line, 1);

  serialPrintln(F("start"));

  while (!timeOutReached(timer)) {
    backgroundtasks();
  }
  serialPrintln(F("end"));
  return return_see_serial(event);
}
#endif // BUILD_NO_DIAGNOSTIC_COMMANDS

const __FlashStringHelper * Command_Debug(struct EventStruct *event, const char *Line)
{
  if (HasArgv(Line, 2)) {
    setLogLevelFor(LOG_TO_SERIAL, parseCommandArgumentInt(Line, 1));
  } else  {
    serialPrintln();
    serialPrint(F("Serial debug level: "));
    serialPrintln(String(Settings.SerialLogLevel));
  }
  return return_see_serial(event);
}

const __FlashStringHelper * Command_logentry(struct EventStruct *event, const char *Line)
{
  uint8_t level = LOG_LEVEL_INFO;
  // An extra optional parameter to set log level.
  if (event->Par2 > LOG_LEVEL_NONE && event->Par2 <= 
  # ifndef BUILD_NO_DEBUG
    LOG_LEVEL_DEBUG_MORE
  #else
    LOG_LEVEL_INFO
  #endif
    ) { level = event->Par2; }
  addLog(level, tolerantParseStringKeepCase(Line, 2));
  return return_command_success();
}

#ifndef BUILD_NO_DIAGNOSTIC_COMMANDS
const __FlashStringHelper * Command_JSONPortStatus(struct EventStruct *event, const char *Line)
{
  addLog(LOG_LEVEL_INFO, F("JSON Port Status: Command not implemented yet."));
  return return_command_success();
}

void createLogPortStatus(std::map<uint32_t, portStatusStruct>::iterator it)
{  
  if (loglevelActiveFor(LOG_LEVEL_INFO)) {
    String log;
    log += F("PortStatus detail: Port=");
    log += getPortFromKey(it->first);
    log += F(" State=");
    log += it->second.state;
    log += F(" Output=");
    log += it->second.output;
    log += F(" Mode=");
    log += it->second.mode;
    log += F(" Task=");
    log += it->second.task;
    log += F(" Monitor=");
    log += it->second.monitor;
    log += F(" Command=");
    log += it->second.command;
    log += F(" Init=");
    log += it->second.init;
    log += F(" PreviousTask=");
    log += it->second.previousTask;
    addLogMove(LOG_LEVEL_INFO, log);
  }
}

void debugPortStatus(std::map<uint32_t, portStatusStruct>::iterator it)
{
  createLogPortStatus(it);
}


const __FlashStringHelper * Command_logPortStatus(struct EventStruct *event, const char *Line)
{
  if (loglevelActiveFor(LOG_LEVEL_INFO)) {
    addLogMove(LOG_LEVEL_INFO, concat(F("PortStatus structure: Called from=Rules Count="), static_cast<int>(globalMapPortStatus.size())));
  }

  for (std::map<uint32_t, portStatusStruct>::iterator it = globalMapPortStatus.begin(); it != globalMapPortStatus.end(); ++it) {
    debugPortStatus(it);
  }

  return return_command_success();
}
#endif // BUILD_NO_DIAGNOSTIC_COMMANDS

#include "../Commands/Blynk.h"

#ifdef USES_C012

#include "../Commands/Common.h"
#include "../DataStructs/ESPEasy_EventStruct.h"
#include "../ESPEasyCore/ESPEasy_backgroundtasks.h"
#include "../ESPEasyCore/ESPEasy_Log.h"
#include "../Globals/Protocol.h"
#include "../Globals/Settings.h"
#include "../Helpers/ESPEasy_Storage.h"
#include "../Helpers/ESPEasy_time_calc.h"
#include "../Helpers/_CPlugin_Helper.h"

#include "../../ESPEasy_fdwdecl.h"


controllerIndex_t firstEnabledBlynk_ControllerIndex() {
  for (controllerIndex_t i = 0; i < CONTROLLER_MAX; ++i) {
    protocolIndex_t ProtocolIndex = getProtocolIndex_from_ControllerIndex(i);

    if (validProtocolIndex(ProtocolIndex)) {
      if ((Protocol[ProtocolIndex].Number == 12) && Settings.ControllerEnabled[i]) {
        return i;
      }
    }
  }
  return INVALID_CONTROLLER_INDEX;
}

const __FlashStringHelper * Command_Blynk_Get(struct EventStruct *event, const char *Line)
{
  controllerIndex_t first_enabled_blynk_controller = firstEnabledBlynk_ControllerIndex();

  if (!validControllerIndex(first_enabled_blynk_controller)) {
    return F("Controller not enabled");
  } else {
    // FIXME TD-er: This one is not using parseString* function
    String strLine = Line;
    strLine = strLine.substring(9);
    int index = strLine.indexOf(',');

    if (index > 0)
    {
      int index           = strLine.lastIndexOf(',');
      String blynkcommand = strLine.substring(index + 1);
      float  value        = 0;

      if (Blynk_get(blynkcommand, first_enabled_blynk_controller, &value))
      {
        UserVar[(VARS_PER_TASK * (event->Par1 - 1)) + event->Par2 - 1] = value;
      }
      else {
        return F("Error getting data");
      }
    }
    else
    {
      if (!Blynk_get(strLine, first_enabled_blynk_controller, nullptr))
      {
        return F("Error getting data");
      }
    }
  }
  return return_command_success();
}

bool Blynk_get(const String& command, controllerIndex_t controllerIndex, float *data)
{
  bool MustCheckReply = false;
  String hostname, pass;
  unsigned int ClientTimeout = 0;
  WiFiClient client;

  {
    // Place ControllerSettings in its own scope, as it is quite big.
    MakeControllerSettings(ControllerSettings); //-V522
    if (!AllocatedControllerSettings()) {
      addLog(LOG_LEVEL_ERROR, F("Blynk : Cannot run GET, out of RAM"));
      return false;
    }

    LoadControllerSettings(controllerIndex, ControllerSettings);
    MustCheckReply = ControllerSettings.MustCheckReply;
    hostname = ControllerSettings.getHost();
    pass = getControllerPass(controllerIndex, ControllerSettings);
    ClientTimeout = ControllerSettings.ClientTimeout;

    if (pass.isEmpty()) {
      addLog(LOG_LEVEL_ERROR, F("Blynk : No password set"));
      return false;
    }

    if (!try_connect_host(/* CPLUGIN_ID_012 */ 12, client, ControllerSettings)) {
      return false;
    }
  }

  // We now create a URI for the request
  {
    // Place this stack allocated array in its own scope, as it is quite big.
    char request[300] = { 0 };
    sprintf_P(request,
              PSTR("GET /%s/%s HTTP/1.1\r\n Host: %s \r\n Connection: close\r\n\r\n"),
              pass.c_str(),
              command.c_str(),
              hostname.c_str());
#ifndef BUILD_NO_DEBUG
    addLog(LOG_LEVEL_DEBUG, request);
#endif
    client.print(request);
  }
  bool success = !MustCheckReply;

  if (MustCheckReply || data) {
    unsigned long timer = millis() + ClientTimeout;

    while (!client_available(client) && !timeOutReached(timer)) {
      delay(1);
    }

    #ifndef BUILD_NO_DEBUG
    char log[80] = { 0 };
    #endif
    timer = millis() + 1500;

    // Read all the lines of the reply from server and log them
    while (client_available(client) && !success && !timeOutReached(timer)) {
      String line;
      safeReadStringUntil(client, line, '\n');
      #ifndef BUILD_NO_DEBUG
      addLog(LOG_LEVEL_DEBUG_MORE, line);
      #endif

      // success ?
      if (line.substring(0, 15).equals(F("HTTP/1.1 200 OK"))) {
        #ifndef BUILD_NO_DEBUG
        strcpy_P(log, PSTR("HTTP : Success"));
        #endif

        if (!data) { success = true; }
      }
      #ifndef BUILD_NO_DEBUG
      else if (line.substring(0, 24).equals(F("HTTP/1.1 400 Bad Request"))) {
        strcpy_P(log, PSTR("HTTP : Unauthorized"));
      }
      else if (line.substring(0, 25).equals(F("HTTP/1.1 401 Unauthorized"))) {
        strcpy_P(log, PSTR("HTTP : Unauthorized"));
      }
      addLog(LOG_LEVEL_DEBUG, log);
      #endif

      // data only
      if (data && line.startsWith("["))
      {
        String strValue = line;
        uint8_t   pos      = strValue.indexOf('"', 2);
        strValue = strValue.substring(2, pos);
        strValue.trim();
        *data   = 0.0f;
        validFloatFromString(strValue, *data);
        success = true;

        char value_char[5] = { 0 };
        strValue.toCharArray(value_char, 5);
        #ifndef BUILD_NO_DEBUG
        sprintf_P(log, PSTR("Blynk get - %s => %s"), command.c_str(), value_char);
        addLog(LOG_LEVEL_DEBUG, log);
        #endif
      }
      delay(0);
    }
  }
  #ifndef BUILD_NO_DEBUG
  addLog(LOG_LEVEL_DEBUG, F("HTTP : closing connection (012)"));
  #endif

  client.flush();
  client.stop();

  // important - backgroundtasks - free mem
  unsigned long timer = millis() + 10;

  while (!timeOutReached(timer)) {
    backgroundtasks();
  }

  return success;
}

#endif // ifdef USES_C012

#include "../Commands/WiFi.h"

#include "../../ESPEasy_common.h"

#include "../Commands/Common.h"

#include "../DataStructs/ESPEasy_EventStruct.h"

#include "../ESPEasyCore/ESPEasyWifi.h"
#include "../ESPEasyCore/Serial.h"

#include "../Globals/ESPEasyWiFiEvent.h"
#include "../Globals/RTC.h"
#include "../Globals/Settings.h"

#include "../Helpers/StringConverter.h"


#define WIFI_MODE_MAX (WiFiMode_t)4


String Command_Wifi_SSID(struct EventStruct *event, const char *Line)
{
  return Command_GetORSetString(event, F("Wifi SSID:"),
                                Line,
                                SecuritySettings.WifiSSID,
                                sizeof(SecuritySettings.WifiSSID),
                                1);
}

String Command_Wifi_Key(struct EventStruct *event, const char *Line)
{
  return Command_GetORSetString(event, F("Wifi Key:"),
                                Line,
                                SecuritySettings.WifiKey,
                                sizeof(SecuritySettings.WifiKey),
                                1);
}

String Command_Wifi_SSID2(struct EventStruct *event, const char *Line)
{
  return Command_GetORSetString(event, F("Wifi2 SSID:"),
                                Line,
                                SecuritySettings.WifiSSID2,
                                sizeof(SecuritySettings.WifiSSID2),
                                1);
}

String Command_Wifi_Key2(struct EventStruct *event, const char *Line)
{
  return Command_GetORSetString(event, F("Wifi2 Key:"),
                                Line,
                                SecuritySettings.WifiKey2,
                                sizeof(SecuritySettings.WifiKey2),
                                1);
}

const __FlashStringHelper * Command_Wifi_Scan(struct EventStruct *event, const char *Line)
{
  WiFiScan_log_to_serial();
  return return_command_success();
}

const __FlashStringHelper * Command_Wifi_Connect(struct EventStruct *event, const char *Line)
{
  WiFiEventData.wifiConnectAttemptNeeded = true;
  return return_command_success();
}

const __FlashStringHelper * Command_Wifi_Disconnect(struct EventStruct *event, const char *Line)
{
  RTC.clearLastWiFi(); // Force a WiFi scan
  WifiDisconnect();

  return return_command_success();
}

const __FlashStringHelper * Command_Wifi_APMode(struct EventStruct *event, const char *Line)
{
  setAP(true);
  return return_command_success();
}

const __FlashStringHelper * Command_Wifi_STAMode(struct EventStruct *event, const char *Line)
{
  setSTA(true);
  return return_command_success();
}

String Command_Wifi_Mode(struct EventStruct *event, const char *Line)
{
  String TmpStr1;

  if (GetArgv(Line, TmpStr1, 2)) {
    WiFiMode_t mode = WIFI_MODE_MAX;

    if (event->Par1 > 0 && event->Par1 < WIFI_MODE_MAX) {
      mode = static_cast<WiFiMode_t>(event->Par1 - 1);
    } else {
      TmpStr1.toLowerCase();

      if (strcmp_P(TmpStr1.c_str(), PSTR("off")) == 0) { mode = WIFI_OFF; }
      else if (strcmp_P(TmpStr1.c_str(), PSTR("sta")) == 0) { mode = WIFI_STA; }
      else if (strcmp_P(TmpStr1.c_str(), PSTR("ap")) == 0) { mode = WIFI_AP; }
      else if (strcmp_P(TmpStr1.c_str(), PSTR("ap+sta")) == 0) { mode = WIFI_AP_STA; }
    }

    if ((mode >= WIFI_OFF) && (mode < WIFI_MODE_MAX)) {
      setWifiMode(mode);
    } else {
      return return_result(event, F("Wifi Mode: invalid arguments"));
    }
  } else {
    return return_result(event, concat(F("WiFi Mode:"),  getWifiModeString(WiFi.getMode())));
  }
  return return_command_success_str();
}

const __FlashStringHelper * Command_Wifi_AllowAP(struct EventStruct *event, const char* Line)
{
  Settings.DoNotStartAP(false);
  return return_command_success();
}

// FIXME: TD-er This is not an erase, but actually storing the current settings
// in the wifi settings of the core library
const __FlashStringHelper * Command_WiFi_Erase(struct EventStruct *event, const char *Line)
{
  WiFi.persistent(true);  // use SDK storage of SSID/WPA parameters
  WifiDisconnect();       // this will store empty ssid/wpa into sdk storage
  WiFi.persistent(false); // Do not use SDK storage of SSID/WPA parameters
  return return_command_success();
}

#include "../Commands/Settings.h"

#include "../../ESPEasy_common.h"

#include "../Commands/Common.h"

#include "../CustomBuild/CompiletimeDefines.h"

#include "../ESPEasyCore/ESPEasyNetwork.h"
#include "../ESPEasyCore/Serial.h"

#include "../Globals/SecuritySettings.h"
#include "../Globals/Settings.h"

#include "../Helpers/ESPEasy_FactoryDefault.h"
#include "../Helpers/ESPEasy_Storage.h"
#include "../Helpers/Memory.h"
#include "../Helpers/Misc.h"
#include "../Helpers/StringConverter.h"
#include "../Helpers/StringGenerator_System.h"


String Command_Settings_Build(struct EventStruct *event, const char* Line)
{
	if (HasArgv(Line, 2)) {
	  Settings.Build = event->Par1;
	} else {
      return return_result(event, concat(F("Build:"), static_cast<int>(Settings.Build)));
	}
	return return_command_success_str();
}

String Command_Settings_Unit(struct EventStruct *event, const char* Line)
{
	if (HasArgv(Line, 2)) {
		Settings.Unit = event->Par1;
	}else  {
      return return_result(event, concat(F("Unit:"), static_cast<int>(Settings.Unit)));
	}
	return return_command_success_str();
}

String Command_Settings_Name(struct EventStruct *event, const char* Line)
{
	return Command_GetORSetString(event, F("Name:"),
				      Line,
				      Settings.Name,
				      sizeof(Settings.Name),
				      1);
}

String Command_Settings_Password(struct EventStruct *event, const char* Line)
{
	return Command_GetORSetString(event, F("Password:"),
				      Line,
				      SecuritySettings.Password,
				      sizeof(SecuritySettings.Password),
				      1
				      );
}

const __FlashStringHelper *  Command_Settings_Password_Clear(struct EventStruct *event, const char* Line)
{
	const String storedPassword = SecuritySettings.getPassword();
	if (storedPassword.length() > 0) {
		// There is a password set, so we must check it.
		const String password = parseStringKeepCase(Line, 2);
		if (!storedPassword.equals(password)) {
			return return_command_failed();
		}
        ZERO_FILL(SecuritySettings.Password);
	}
	return return_command_success();
}

const __FlashStringHelper * Command_Settings_Save(struct EventStruct *event, const char* Line)
{
	SaveSettings();
	return return_command_success();
}

const __FlashStringHelper * Command_Settings_Load(struct EventStruct *event, const char* Line)
{
	LoadSettings();
	return return_command_success();
}

const __FlashStringHelper * Command_Settings_Print(struct EventStruct *event, const char* Line)
{
	serialPrintln();

	serialPrintln(F("System Info"));
	serialPrint(F("  IP Address    : ")); serialPrintln(NetworkLocalIP().toString());
	serialPrint(F("  Build         : ")); serialPrintln(String(get_build_nr()) + '/' + getSystemBuildString());
	serialPrint(F("  Name          : ")); serialPrintln(Settings.Name);
	serialPrint(F("  Unit          : ")); serialPrintln(String(static_cast<int>(Settings.Unit)));
	serialPrint(F("  WifiSSID      : ")); serialPrintln(SecuritySettings.WifiSSID);
	serialPrint(F("  WifiKey       : ")); serialPrintln(SecuritySettings.WifiKey);
	serialPrint(F("  WifiSSID2     : ")); serialPrintln(SecuritySettings.WifiSSID2);
	serialPrint(F("  WifiKey2      : ")); serialPrintln(SecuritySettings.WifiKey2);
	serialPrint(F("  Free mem      : ")); serialPrintln(String(FreeMem()));
	return return_see_serial(event);
}

const __FlashStringHelper * Command_Settings_Reset(struct EventStruct *event, const char* Line)
{
	ResetFactory();
	reboot(ESPEasy_Scheduler::IntendedRebootReason_e::ResetFactoryCommand);
	return return_command_success();
}

#include "../Commands/Time.h"


#include "../../ESPEasy_common.h"


#include "../Commands/Common.h"

#include "../DataStructs/ESPEasy_EventStruct.h"

#include "../ESPEasyCore/Serial.h"

#include "../Globals/ESPEasy_time.h"
#include "../Globals/Settings.h"

#include "../Helpers/ESPEasy_time_calc.h"
#include "../Helpers/StringConverter.h"


String Command_NTPHost(struct EventStruct *event, const char *Line)
{
  return Command_GetORSetString(event, F("NTPHost:"),
                                Line,
                                Settings.NTPHost,
                                sizeof(Settings.NTPHost),
                                1);
}

String Command_useNTP(struct EventStruct *event, const char *Line)
{
  if (HasArgv(Line, 2)) {
    Settings.UseNTP(event->Par1);
  } else {
    return return_result(event, concat(F("UseNTP:"), boolToString(Settings.UseNTP())));
  }
  return return_command_success_str();
}

String Command_TimeZone(struct EventStruct *event, const char *Line)
{
  if (HasArgv(Line, 2)) {
    Settings.TimeZone = event->Par1;
  } else {
    return return_result(event, concat(F("TimeZone:"), static_cast<int>(Settings.TimeZone)));
  }
  return return_command_success_str();
}

String Command_DST(struct EventStruct *event, const char *Line)
{
  if (HasArgv(Line, 2)) {
    Settings.DST = event->Par1;
  } else  {
    return return_result(event, concat(F("DST:"),  boolToString(Settings.DST)));
  }
  return return_command_success_str();
}

String Command_DateTime(struct EventStruct *event, const char *Line)
{
  String TmpStr1;

  if (GetArgv(Line, TmpStr1, 2)) {
    struct tm newtime;
    int yr = 1970;
    int mnth = 1;
    int d = 1;
    sscanf(TmpStr1.c_str(), "%4d-%2d-%2d", &yr, &mnth, &d);
    newtime.tm_year = yr - 1900;
    newtime.tm_mon  = mnth - 1; // tm_mon starts at 0
    newtime.tm_mday = d;

    int h = 0;
    int m = 0;
    int s = 0;

    if (GetArgv(Line, TmpStr1, 3)) {
      sscanf(TmpStr1.c_str(), "%2d:%2d:%2d", &h, &m, &s);
    }
    newtime.tm_hour = h;
    newtime.tm_min  = m;
    newtime.tm_sec  = s;

    // Please note the time set in this command is in UTC time, not local time.
    node_time.setExternalTimeSource(makeTime(newtime), timeSource_t::Manual_set);
  } else  {
    // serialPrintln();
    return return_result(event, concat(F("Datetime:"), node_time.getDateTimeString('-', ':', ' ')));
  }
  return return_command_success_str();
}
