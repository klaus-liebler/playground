#pragma once
#include <array>
#include <vector>
#include <semaphore>
#include <mutex>
#include <chrono>
#include "./i_log.hh"
#include "./i_uart.hh"
namespace atparser
{
    constexpr char AT_PARAM_SEPARATOR{','};
    constexpr char AT_RSP_SEPARATOR{':'};
    constexpr char AT_CMD_SEPARATOR{'='};
    constexpr char AT_CMD_READ_TEST_IDENTIFIER{'?'};
    constexpr char AT_CMD_BUFFER_TERMINATOR{0};
    constexpr char AT_CMD_STRING_IDENTIFIER{'\"'};
    constexpr char AT_STANDARD_NOTIFICATION_PREFIX{'+'};
    constexpr std::chrono::milliseconds TIMEOUT{5000};
    constexpr size_t AT_CMD_MAX_ARRAY_SIZE{64};
    constexpr size_t MAX_FULL_LINE_ANSWER_BUFFER_LEN{64};

    constexpr const char *TAG = "uart_events";

    enum class ErrorCode
    {
        OK,
        INVALID_ARGUMENTS,
        DTE_NOT_READY,
        TIMEOUT,
        ERROR,
    };

    constexpr const std::array RESULT_PREFIXES = {
        "OK",
        "ERROR",
        "+CME ERROR",
        "+CMS ERROR",
    };

    constexpr const std::array CPIN_ANSWERS = {
        "READY",
        "SIM PIN",
        "SIM PUK",
        "PH_SIM PIN",
        "PH_SIM PUK",
        "PH_NET PIN",
        "SIM PIN2",
        "SIM PUK2",
        "MAX",
    };

    enum class eCPINAnswers
    {
        READY,
        SIM_PIN,
        SIM_PUK,
        PH_SIM_PIN,
        PH_SIM_PUK,
        PH_NET_PIN,
        SIM_PIN2,
        SIM_PUK2,
        MAX,
    };

    bool starts_with(char *testee, const char *prefix, char **next)
    {
        while (*prefix)
        {
            if (*prefix++ != *testee++)
                return false;
        }
        if (next)
        {
            *next = testee;
        }
        return true;
    }

    enum class eParamType
    {
        NUM_INT,
        NUM_INT_ARRAY,
        STRING_QUOTED,
        STRING_NON_QUOTED,
    };

    enum class CME_ERRORS
    {
        phone_failure = 0,
        no_connection_to_phone = 1,
        phone_adaptor_link_reserved = 2,
        operation_not_allowed = 3,
        operation_not_supported = 4,
        PH_SIM_PIN_required = 5,
        PH_FSIM_PIN_required = 6,
        PH_FSIM_PUK_required = 7,
        SIM_not_inserted = 10,
        SIM_PIN_required = 11,
        SIM_PUK_required = 12,
    };

    enum class eAnswerType
    {
        UNDEFINED,
        WITH_COMMAND_PREFIX,
        FULL_LINE_WITHOUT_PREFIX,
    };

    class cAbstactParam
    {
    public:
        eParamType paramType;
        bool optional;
        virtual bool Parse(char **str) = 0;

    protected:
        cAbstactParam(eParamType paramType, bool optional) : paramType(paramType), optional(optional) {}

        static bool is_array_start(char chr)
        {
            return (chr == '(');
        }

        static bool is_array_stop(char chr)
        {
            return (chr == ')');
        }

        static bool is_terminated(char c)
        {
            return c == '\0';
        }

        static bool is_separator(char c)
        {
            return c == ',';
        }
    };

    class cCPINParam : public cAbstactParam
    {
    public:
        eCPINAnswers value;
        bool Parse(char **c) override
        {
            char *pos = *c;
            value = eCPINAnswers::MAX;
            for (size_t i = 0; i < (size_t)eCPINAnswers::MAX; i++)
            {
                if (starts_with(pos, CPIN_ANSWERS[i], nullptr))
                {
                    value = (eCPINAnswers)i;
                    break;
                }
            }
            LOGI(TAG, "Parsed eCPINAnswers %d", (int)value);
            return value != eCPINAnswers::MAX;
        }
    };

    class cNumIntArrayParam : public cAbstactParam
    {
    public:
        cNumIntArrayParam(bool optional) : cAbstactParam(eParamType::NUM_INT_ARRAY, optional) {}
        uint32_t *value;
        bool Parse(char **c) override
        {
            char *pos = *c;
            char *next;
            size_t i = 0;
            uint32_t tmparray[AT_CMD_MAX_ARRAY_SIZE];

            if (!is_array_start(*pos))
                return false;
            pos++;
            tmparray[i++] = (uint32_t)strtoul(pos, &next, 10);
            pos = next;

            while (!is_array_stop(*pos) && !is_terminated(*pos) && i < AT_CMD_MAX_ARRAY_SIZE)
            {
                if (!is_separator(*pos))
                    return false;
                pos++;
                tmparray[i++] = (uint32_t)strtoul(pos, &next, 10);
                pos = next;
            }
            value = new uint32_t[i];
            for (int j = 0; j < i; j++)
            {
                value[j] = tmparray[j];
            }
            *c = pos;
            LOGI(TAG, "Parsed cNumIntArrayParam with len %d", i);
            return true;
        }

        ~cNumIntArrayParam()
        {
            delete[] value;
        }
    };

    class cStringQuotedParam : public cAbstactParam
    {
    public:
        cStringQuotedParam(bool optional) : cAbstactParam(eParamType::STRING_QUOTED, optional) {}
        char *value;
        bool Parse(char **c) override
        {
            char *pos = *c;
            if (*pos++ != '"')
                return false;
            char *start = pos;
            while (*pos++ != '"')
                ;
            size_t len = pos - start;
            value = (char *)malloc(len + 1);
            strncpy(value, start, len);
            value[len] = '\0';
            *c = pos;
            LOGI(TAG, "Parsed cStringQuotedParam %s with len %d", value, len);
            return true;
        }
        ~cStringQuotedParam()
        {
            LOGI(TAG, "About to destroy string %s", value);
            free(value);
        }
    };

    class cStringNonQuotedParam : public cAbstactParam
    {
    public:
        cStringNonQuotedParam(bool optional) : cAbstactParam(eParamType::STRING_NON_QUOTED, optional) {}
        char *value;
        bool Parse(char **c) override
        {
            char *pos = *c;
            char *start = pos;
            while (*pos++ != '\0')
                ;
            size_t len = pos - start;
            value = (char *)malloc(len + 1);
            strncpy(value, start, len);
            value[len] = '\0';
            *c = pos;
            LOGI(TAG, "Parsed cStringNonQuoted %s with len %d", value, len);
            return true;
        }
        ~cStringNonQuotedParam()
        {
            LOGI(TAG, "About to destroy string %s", value);
            free(value);
        }
    };

    class cNumIntParam : public cAbstactParam
    {
    public:
        cNumIntParam(bool optional) : cAbstactParam(eParamType::NUM_INT, optional) {}
        uint64_t value;
        bool Parse(char **c) override
        {
            char *next;
            int64_t value = (int64_t)strtoll(*c, &next, 10);
            *c = next;
            return true;
        }
        ~cNumIntParam()
        {
        }
    };

    enum class at_cmd_type
    {
        AT_CMD_TYPE_UNKNOWN,
        AT_CMD_TYPE_TEST_COMMAND,
        AT_CMD_TYPE_READ_COMMAND,
        AT_CMD_TYPE_SET_COMMAND,
    };

    class Parser
    {
    private:
        bool readyReceived{false};
        std::mutex commandMutex; // only one single command may be "inflight"
        std::binary_semaphore waitForResultSemaphore{1};

        const char *currentCommand;
        char currentFullLineAnswer[MAX_FULL_LINE_ANSWER_BUFFER_LEN + 1];
        eAnswerType currentAnswerType{eAnswerType::WITH_COMMAND_PREFIX};
        std::vector<cAbstactParam *> *currentParams;

    public:
        Parser(){}

        void Init()
        {
        }

        void Parse(char *line)
        {

            // wenn es mit + oder * beginnt -->parsen
            // wenn es OK oder ERROR oder RDY ist -->direkt erkennen
            // wenn wir im FULL_LINE_WITHOUT_PREFIX Answer-Modus sind: Kompletten string einlesen und abspeichern
            if (currentAnswerType == eAnswerType::FULL_LINE_WITHOUT_PREFIX)
            {
                strncpy(currentFullLineAnswer, line, MAX_FULL_LINE_ANSWER_BUFFER_LEN);
                currentAnswerType = eAnswerType::UNDEFINED;
                return;
            }
            char *next{nullptr};
            switch (line[0])
            {
            case '*': //*PSUTTZ
                if (strncmp(line, "*PSUTTZ", 7) == 0)
                {
                    LOGI(TAG, "Got an updated time");
                }
                break;
            case '+':

                if (!starts_with(line + 1, currentCommand, &next))
                {
                    LOGE(TAG, "Unexpected respose: %s", line);
                    errno = EIO;

                    waitForResultSemaphore.release();
                }
                if (*next != ':')
                {
                    LOGE(TAG, "No colon found: %s", line);
                    errno = EIO;
                    waitForResultSemaphore.release();
                }
                next++;
                for (int i = 0; i < currentParams->size(); i++)
                {
                    bool success = currentParams->at(i)->Parse(&next);
                    if (!success)
                    {
                        LOGE(TAG, "Error while parsing parameter %d of %s", i, line);
                        errno = EIO;
                        waitForResultSemaphore.release();
                    }
                }
                break;
            case 'D': // DST
                break;
            case 'E': // ERROR
                if (strcmp(line, "ERROR") == 0)
                {
                    LOGI(TAG, "Device returned ERROR");
                    errno = 1;
                    waitForResultSemaphore.release();
                }
                break;
            case 'N': // NORMAL POWER DOWN
                break;
            case 'O': // OK, OVER VOLTAGE
                if (strcmp(line, "OK") == 0)
                {
                    LOGI(TAG, "Device returned OK");
                    errno = 0;
                    waitForResultSemaphore.release();
                }
                break;
            case 'R': // RDY
                if (strcmp(line, "RDY") == 0)
                {
                    LOGI(TAG, "Device is RDY");
                    this->readyReceived=true;
                }
                break;
            case 'S': // SMS Ready
                if (strcmp(line, "SMS Ready") == 0)
                {
                    LOGI(TAG, "SMS Ready!");
                }
                break;
            case 'U': // UNDER VOLTAGE
                break;

            default:
                LOGW(TAG, "Unknown first character 0x%02X in line %s", line[0], line);
                break;
            }
        }

        ErrorCode WriteCommandAndWaitResult(const char *command, std::vector<cAbstactParam *> &params, iUartHal *uartHal)
        {
            if (!command)
                return ErrorCode::INVALID_ARGUMENTS;
            if (!this->readyReceived)
            {
                return ErrorCode::DTE_NOT_READY;
            }
            const std::lock_guard<std::mutex> lock(commandMutex); // owns the mutex for the duration of the scoped block.
            // now, there is no other inflight command
            currentCommand = command;
            currentParams = &params;
            waitForResultSemaphore.try_acquire_for(TIMEOUT);
            uartHal->writeChars(command, strlen(command));
            uartHal->writeChars("\r\n", 2);
            if(!waitForResultSemaphore.try_acquire_for(TIMEOUT)){
                waitForResultSemaphore.release();
                return ErrorCode::TIMEOUT;
            }
            waitForResultSemaphore.release();
            return ErrorCode::OK;
        }
    };

}
