#include <dirent.h>
#include <stdio.h>
#include <string.h>

#include "console/cli.h"
#include "console/config.h"
#include "console/debug.h"
#include "version.h"

/**
 * Print usage information
 * 
 * Prints the usage information for the OpenJVS command
 * line interface
 * 
 * @returns The status of the action performed
 **/
JVSCLIStatus printUsage()
{
    debug(0, "Usage: openjvs ( options [controller] | [game] )\n\n");
    debug(0, "Options:\n");
    debug(0, "  --list     Lists all controllers\n");
    debug(0, "  --edit     Opens a file for editing\n");
    debug(0, "  --enable   Enables a new/all controller(s)\n");
    debug(0, "  --disable  Disables a new/all controller(s)\n");
    debug(0, "  --help     Displays this text\n");
    debug(0, "  --debug    Runs in debug mode\n");
    debug(0, "  --version  Displays the OpenJVS Version\n");
    return JVS_CLI_STATUS_SUCCESS_CLOSE;
}

/**
 * Print version information
 * 
 * Prints the version information for the OpenJVS command
 * line interface
 * 
 * @returns The status of the action performed
 **/
JVSCLIStatus printVersion()
{
    debug(0, "%s\n", PROJECT_VER);
    return JVS_CLI_STATUS_SUCCESS_CLOSE;
}

/**
 * Edit a file in vim
 * 
 * Edits a file in the vim editor
 *
 * @returns The status of the action performed
 **/
JVSCLIStatus editFile(char *filePath)
{
    char mainName[1024];
    strcpy(mainName, DEFAULT_DEVICE_MAPPING_PATH);
    strcat(mainName, filePath);
    if (access(mainName, F_OK) != 0)
    {
        strcpy(mainName, DEFAULT_GAME_MAPPING_PATH);
        strcat(mainName, filePath);
        if (access(mainName, F_OK) != 0)
        {
            printf("Error: Could not find a game or device file with that name\n");
            return JVS_CLI_STATUS_ERROR;
        }
    }

    char command[1024];
    strcpy(command, "sudo editor ");
    strcat(command, mainName);
    system(command);
    return JVS_CLI_STATUS_SUCCESS_CLOSE;
}

/**
 * Enables a device or all devices
 * 
 * Enables a specific device if specified. If not,
 * it will enable all devices in the 
 * DEFAULT_DEVICE_MAPPING_PATH directory.
 * 
 * @param deviceName The name of the device to enable
 * @returns The status of the action performed
 */
JVSCLIStatus enableDevice(char *deviceName)
{
    if (!deviceName)
    {
        DIR *d;
        struct dirent *dir;
        d = opendir(DEFAULT_DEVICE_MAPPING_PATH);
        if (d)
        {
            while ((dir = readdir(d)) != NULL)
            {
                char gamePath[MAX_PATH_LENGTH];
                strcpy(gamePath, DEFAULT_DEVICE_MAPPING_PATH);
                strcat(gamePath, dir->d_name);

                char gamePathEnabled[MAX_PATH_LENGTH];
                strcpy(gamePathEnabled, gamePath);

                for (int i = 0; i < MAX_PATH_LENGTH; i++)
                {
                    if (gamePathEnabled[i] == '.')
                    {
                        gamePathEnabled[i] = 0;
                        break;
                    }
                }

                rename(gamePath, gamePathEnabled);
            }
            closedir(d);
        }

        debug(0, "OpenJVS has enabled all controllers.\n");
        return JVS_CLI_STATUS_SUCCESS_CLOSE;
    }

    char gamePath[MAX_PATH_LENGTH];
    strcpy(gamePath, DEFAULT_DEVICE_MAPPING_PATH);
    strcat(gamePath, deviceName);

    char gamePathDisabled[MAX_PATH_LENGTH];
    strcpy(gamePathDisabled, gamePath);
    strcat(gamePathDisabled, ".disabled");

    if (rename(gamePathDisabled, gamePath) < 0)
    {
        debug(0, "Failed to enable the controller, does it exist and is it already enabled?\n");
        return JVS_CLI_STATUS_ERROR;
    }

    debug(0, "OpenJVS has enabled the controller '%s'.\n", deviceName);
    return JVS_CLI_STATUS_SUCCESS_CLOSE;
}

/**
 * Disables a device or all devices
 * 
 * Disables a specific device if specified. If not,
 * it will disable all devices in the 
 * DEFAULT_DEVICE_MAPPING_PATH directory.
 * 
 * @param deviceName The name of the device to disable
 * @returns The status of the action performed
 */
JVSCLIStatus disableDevice(char *deviceName)
{
    if (!deviceName)
    {
        DIR *d;
        struct dirent *dir;
        d = opendir(DEFAULT_DEVICE_MAPPING_PATH);
        if (d)
        {
            while ((dir = readdir(d)) != NULL)
            {
                char gamePathEnabled[MAX_PATH_LENGTH];
                strcpy(gamePathEnabled, DEFAULT_DEVICE_MAPPING_PATH);
                strcat(gamePathEnabled, dir->d_name);

                char gamePathDisabled[MAX_PATH_LENGTH];
                strcpy(gamePathDisabled, gamePathEnabled);
                strcat(gamePathDisabled, ".disabled");

                rename(gamePathEnabled, gamePathDisabled);
            }
            closedir(d);
        }

        debug(0, "OpenJVS has disabled all controllers.\n");
        return JVS_CLI_STATUS_SUCCESS_CLOSE;
    }

    char gamePath[MAX_PATH_LENGTH];
    strcpy(gamePath, DEFAULT_DEVICE_MAPPING_PATH);
    strcat(gamePath, deviceName);

    char gamePathDisabled[MAX_PATH_LENGTH];
    strcpy(gamePathDisabled, gamePath);
    strcat(gamePathDisabled, ".disabled");

    if (rename(gamePath, gamePathDisabled) < 0)
    {
        debug(0, "Failed to disable the controller, does it exist and is it already disabled?\n");
        return JVS_CLI_STATUS_ERROR;
    }
    debug(0, "OpenJVS has disabled the controller '%s'.\n", deviceName);
    return JVS_CLI_STATUS_SUCCESS_CLOSE;
}

JVSCLIStatus printDeviceListing(Device *device)
{
    printf("  - %s (%s)\n", device->name, device->physicalLocation);

    return JVS_CLI_STATUS_SUCCESS_CONTINUE;
}
/**
 * Prints the listing of devices
 * 
 * Will print out a listing of devices, showing which
 * ones are enabled, disabled and have no mapping present.
 * 
 * @returns The status of the action performed
 **/
JVSCLIStatus printListing()
{
    DeviceList *deviceList = NULL;
    deviceList = malloc(sizeof(DeviceList));

    if (deviceList == NULL)
    {
        debug(0, "Error: Failed to malloc\n");
        return JVS_CLI_STATUS_ERROR;
    }

    if (!getInputs(deviceList))
    {
        debug(0, "OpenJVS failed to detect any controllers.\nMake sure you are running as root.\n");
        return JVS_CLI_STATUS_ERROR;
    }

    debug(0, "OpenJVS can detect the following controllers:\n\n");
    InputMappings inputMappings;
    inputMappings.length = 0;
    debug(0, "Enabled:\n");
    for (int i = 0; i < deviceList->length; i++)
    {
        int enabled = parseInputMapping(deviceList->devices[i].name, &inputMappings) == JVS_CONFIG_STATUS_SUCCESS;
        if (enabled)
        {
            printDeviceListing(&deviceList->devices[i]);
        }
    }
    debug(0, "\nDisabled:\n");
    for (int i = 0; i < deviceList->length; i++)
    {
        char disabledString[MAX_PATH_LENGTH];
        strcpy(disabledString, deviceList->devices[i].name);
        strcat(disabledString, ".disabled");
        int disabled = parseInputMapping(disabledString, &inputMappings) == JVS_CONFIG_STATUS_SUCCESS;
        if (disabled)
        {
            printDeviceListing(&deviceList->devices[i]);
        }
    }
    debug(0, "\nNo Mapping Present:\n");
    for (int i = 0; i < deviceList->length; i++)
    {
        char disabledString[MAX_PATH_LENGTH];
        strcpy(disabledString, deviceList->devices[i].name);
        strcat(disabledString, ".disabled");
        int enabled = parseInputMapping(deviceList->devices[i].name, &inputMappings) == JVS_CONFIG_STATUS_SUCCESS;
        int disabled = parseInputMapping(disabledString, &inputMappings) == JVS_CONFIG_STATUS_SUCCESS;
        if (!enabled && !disabled)
        {
            printDeviceListing(&deviceList->devices[i]);
        }
    }

    if (deviceList != NULL)
    {
        free(deviceList);
        deviceList = NULL;
    }

    return JVS_CLI_STATUS_SUCCESS_CLOSE;
}

/**
 * Parses the command line arguments
 * 
 * Parses the command line arguments and sets the
 * mapping name if no arguments are set.
 * 
 * @param argc The amount of arguments
 * @param argv Structure holding the arguments
 * @param map Pointer to a char array holding the map name
 * @returns The status of the action performed
 **/
JVSCLIStatus parseArguments(int argc, char **argv, char *map)
{
    // If there are no arguments simply continue
    if (argc <= 1)
        return JVS_CLI_STATUS_SUCCESS_CONTINUE;

    // If the first argument doesn't start with a dash it must be a map file.
    if (argv[1][0] != '-')
    {
        strcpy(map, argv[1]);
        return JVS_CLI_STATUS_SUCCESS_CONTINUE;
    }

    // Process all of the different arguments people might send
    if (strcmp(argv[1], "--help") == 0)
    {
        return printUsage();
    }
    else if (strcmp(argv[1], "--version") == 0)
    {
        return printVersion();
    }
    else if (strcmp(argv[1], "--enable") == 0)
    {
        return enableDevice(argc < 3 ? 0 : argv[2]);
    }
    else if (strcmp(argv[1], "--disable") == 0)
    {
        return disableDevice(argc < 3 ? 0 : argv[2]);
    }
    else if (strcmp(argv[1], "--list") == 0)
    {
        return printListing();
    }
    else if (strcmp(argv[1], "--debug") == 0)
    {
        initDebug(1);
        return JVS_CLI_STATUS_SUCCESS_CONTINUE;
    }
    else if (strcmp(argv[1], "--edit") == 0)
    {
        return editFile(argv[2]);
    }

    // If none of these where found, the argument is unknown.
    debug(0, "Unknown argument %s\n", argv[1]);
    return JVS_CLI_STATUS_ERROR;
}
