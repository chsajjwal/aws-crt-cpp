#pragma once
/**
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0.
 */
#include <aws/crt/io/EventLoopGroup.h>
#include <aws/crt/mqtt/MqttClient.h>

#include <aws/iotdevice/device_defender.h>

namespace Aws
{
    namespace Crt
    {

        namespace Io
        {
            class EventLoopGroup;
        }

        namespace Mqtt
        {
            class MqttConnection;
        }

        namespace Iot
        {

            namespace DeviceDefenderV1
            {

                class ReportTask;
                class ReportTaskBuilder;

                /**
                 * Invoked upon DeviceDefender V1 task cancellation.
                 */
                using OnTaskCancelledHandler = std::function<void(void *)>;

                using ReportFormat = aws_iotdevice_defender_report_format;

                /**
                 * Enum used to expose the status of a DeviceDefenderV1 task.
                 */
                enum class ReportTaskStatus
                {
                    Ready = 0,
                    Running = 1,
                    Stopped = 2,
                };

                /**
                 * Represents a persistent DeviceDefender V1 task.
                 */
                class AWS_CRT_CPP_API ReportTask final
                {
                    friend ReportTaskBuilder;

                  public:
                    ~ReportTask();
                    ReportTask(const ReportTask &) = delete;
                    ReportTask(ReportTask &&) noexcept;
                    ReportTask &operator=(const ReportTask &) = delete;
                    ReportTask &operator=(ReportTask &&) noexcept;

                    /**
                     * Initiates stopping of the Defender V1 task.
                     */
                    void StopTask() noexcept;

                    /**
                     * Initiates Defender V1 reporting task.
                     */
                    int StartTask() noexcept;

                    /**
                     * Returns the task status.
                     */
                    ReportTaskStatus GetStatus() noexcept;

                    OnTaskCancelledHandler OnDefenderV1TaskCancelled;

                    void *cancellationUserdata;

                    /**
                     * @return the value of the last aws error encountered by operations on this instance.
                     */
                    int LastError() const noexcept { return m_lastError; }

                  private:
                    Crt::Allocator *m_allocator;
                    ReportTaskStatus m_status;
                    aws_iotdevice_defender_report_task_config m_taskConfig;
                    aws_iotdevice_defender_v1_task *m_owningTask;
                    int m_lastError;

                    ReportTask(
                        Crt::Allocator *allocator,
                        std::shared_ptr<Mqtt::MqttConnection> mqttConnection,
                        const Crt::String &thingName,
                        Io::EventLoopGroup &eventLoopGroup,
                        ReportFormat reportFormat,
                        uint64_t taskPeriodSeconds,
                        uint64_t networkConnectionSamplePeriodSeconds,
                        OnTaskCancelledHandler &&onCancelled = NULL,
                        void *cancellationUserdata = nullptr) noexcept;

                    static void s_onDefenderV1TaskCancelled(void *userData);
                };

                /**
                 * Represents a builder for creating a ReportTask object.
                 */
                class AWS_CRT_CPP_API ReportTaskBuilder final
                {
                  public:
                    ReportTaskBuilder(
                        Crt::Allocator *allocator,
                        std::shared_ptr<Mqtt::MqttConnection> mqttConnection,
                        Io::EventLoopGroup &eventLoopGroup,
                        const Crt::String &thingName);

                    /**
                     * Sets the device defender report format, or defaults to AWS_IDDRF_JSON.
                     */
                    ReportTaskBuilder &WithReportFormat(ReportFormat reportFormat) noexcept;

                    /**
                     * Sets the task period seconds. Defaults to 5 minutes.
                     */
                    ReportTaskBuilder &WithTaskPeriodSeconds(uint64_t taskPeriodSeconds) noexcept;

                    /**
                     * Sets the network connection sample period seconds. Defaults to 5 minutes.
                     */
                    ReportTaskBuilder &WithNetworkConnectionSamplePeriodSeconds(
                        uint64_t networkConnectionSamplePeriodSeconds) noexcept;

                    /**
                     * Sets the task cancelled handler function.
                     */
                    ReportTaskBuilder &WithTaskCancelledHandler(OnTaskCancelledHandler &&onCancelled) noexcept;

                    /**
                     * Sets the user data for the task cancelled handler function.
                     */
                    ReportTaskBuilder &WithTaskCancellationUserData(void *cancellationUserdata) noexcept;

                    /**
                     * Builds a device defender v1 task object from the set options.
                     */
                    ReportTask Build() noexcept;

                  private:
                    Crt::Allocator *m_allocator;
                    std::shared_ptr<Mqtt::MqttConnection> m_mqttConnection;
                    Crt::String m_thingName;
                    Io::EventLoopGroup m_eventLoopGroup;
                    ReportFormat m_reportFormat;
                    uint64_t m_taskPeriodSeconds;
                    uint64_t m_networkConnectionSamplePeriodSeconds;
                    OnTaskCancelledHandler m_onCancelled;
                    void *m_cancellationUserdata;
                };
            } // namespace DeviceDefenderV1

        } // namespace Iot
    }     // namespace Crt
} // namespace Aws