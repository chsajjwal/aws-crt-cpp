/**
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0.
 */
#include <aws/crt/Api.h>

#include <aws/crt/iot/DeviceDefender.h>
#include <aws/testing/aws_test_harness.h>
#include <utility>

static int s_TestDeviceDefenderResourceSafety(Aws::Crt::Allocator *allocator, void *ctx)
{
    (void)ctx;
    {
        Aws::Crt::ApiHandle apiHandle(allocator);
        Aws::Crt::Io::TlsContextOptions tlsCtxOptions = Aws::Crt::Io::TlsContextOptions::InitDefaultClient();

        Aws::Crt::Io::TlsContext tlsContext(tlsCtxOptions, Aws::Crt::Io::TlsMode::CLIENT, allocator);
        ASSERT_TRUE(tlsContext);

        Aws::Crt::Io::SocketOptions socketOptions;
        socketOptions.SetConnectTimeoutMs(3000);

        Aws::Crt::Io::EventLoopGroup eventLoopGroup(0, allocator);
        ASSERT_TRUE(eventLoopGroup);

        Aws::Crt::Io::DefaultHostResolver defaultHostResolver(eventLoopGroup, 8, 30, allocator);
        ASSERT_TRUE(defaultHostResolver);

        Aws::Crt::Io::ClientBootstrap clientBootstrap(eventLoopGroup, defaultHostResolver, allocator);
        ASSERT_TRUE(allocator);
        clientBootstrap.EnableBlockingShutdown();

        Aws::Crt::Mqtt::MqttClient mqttClient(clientBootstrap, allocator);
        ASSERT_TRUE(mqttClient);

        Aws::Crt::Mqtt::MqttClient mqttClientMoved = std::move(mqttClient);
        ASSERT_TRUE(mqttClientMoved);

        auto mqttConnection = mqttClientMoved.NewConnection("www.example.com", 443, socketOptions, tlsContext);

        const Aws::Crt::String thingName("TestThing");
        Aws::Crt::String data("TestData");

        std::mutex mutex;
        std::condition_variable cv;
        bool taskStopped = false;

        auto onCancelled = [&](void *a) {
            auto data = reinterpret_cast<Aws::Crt::String *>(a);
            ASSERT_INT_EQUALS(0, data->compare("TestData"));
            taskStopped = true;
            cv.notify_one();
        };

        Aws::Crt::Iot::DeviceDefenderV1::ReportTaskBuilder taskBuilder(
            allocator, mqttConnection, eventLoopGroup, thingName);
        taskBuilder.WithTaskPeriodSeconds((uint64_t)1UL)
            .WithNetworkConnectionSamplePeriodSeconds((uint64_t)1UL)
            .WithTaskCancelledHandler(onCancelled)
            .WithTaskCancellationUserData(&data);

        Aws::Crt::Iot::DeviceDefenderV1::ReportTask task = taskBuilder.Build();

        ASSERT_INT_EQUALS((int)Aws::Crt::Iot::DeviceDefenderV1::ReportTaskStatus::Ready, (int)task.GetStatus());

        task.StartTask();
        ASSERT_INT_EQUALS((int)Aws::Crt::Iot::DeviceDefenderV1::ReportTaskStatus::Running, (int)task.GetStatus());
        task.StopTask();

        {
            std::unique_lock<std::mutex> lock(mutex);
            cv.wait(lock, [&]() { return taskStopped; });
        }

        mqttConnection->Disconnect();
        ASSERT_TRUE(mqttConnection);

        ASSERT_FALSE(mqttClient);

        ASSERT_INT_EQUALS((int)Aws::Crt::Iot::DeviceDefenderV1::ReportTaskStatus::Stopped, (int)task.GetStatus());
    }

    return AWS_ERROR_SUCCESS;
}

AWS_TEST_CASE(DeviceDefenderResourceSafety, s_TestDeviceDefenderResourceSafety)

static int s_TestDeviceDefenderFailedTest(Aws::Crt::Allocator *allocator, void *ctx)
{
    (void)ctx;
    {
        Aws::Crt::ApiHandle apiHandle(allocator);
        Aws::Crt::Io::TlsContextOptions tlsCtxOptions = Aws::Crt::Io::TlsContextOptions::InitDefaultClient();

        Aws::Crt::Io::TlsContext tlsContext(tlsCtxOptions, Aws::Crt::Io::TlsMode::CLIENT, allocator);
        ASSERT_TRUE(tlsContext);

        Aws::Crt::Io::SocketOptions socketOptions;
        socketOptions.SetConnectTimeoutMs(3000);

        Aws::Crt::Io::EventLoopGroup eventLoopGroup(0, allocator);
        ASSERT_TRUE(eventLoopGroup);

        Aws::Crt::Io::DefaultHostResolver defaultHostResolver(eventLoopGroup, 8, 30, allocator);
        ASSERT_TRUE(defaultHostResolver);

        Aws::Crt::Io::ClientBootstrap clientBootstrap(eventLoopGroup, defaultHostResolver, allocator);
        ASSERT_TRUE(allocator);
        clientBootstrap.EnableBlockingShutdown();

        Aws::Crt::Mqtt::MqttClient mqttClient(clientBootstrap, allocator);
        ASSERT_TRUE(mqttClient);

        Aws::Crt::Mqtt::MqttClient mqttClientMoved = std::move(mqttClient);
        ASSERT_TRUE(mqttClientMoved);

        auto mqttConnection = mqttClientMoved.NewConnection("www.example.com", 443, socketOptions, tlsContext);

        const Aws::Crt::String thingName("TestThing");
        Aws::Crt::String data("TestData");

        Aws::Crt::Iot::DeviceDefenderV1::ReportTaskBuilder taskBuilder(
            allocator, mqttConnection, eventLoopGroup, thingName);
        taskBuilder.WithTaskPeriodSeconds((uint64_t)1UL)
            .WithTaskPeriodSeconds((uint64_t)1UL)
            .WithReportFormat(Aws::Crt::Iot::DeviceDefenderV1::ReportFormat::AWS_IDDRF_SHORT_JSON);

        Aws::Crt::Iot::DeviceDefenderV1::ReportTask task = taskBuilder.Build();

        task.OnDefenderV1TaskCancelled = [](void *a) {
            auto data = reinterpret_cast<Aws::Crt::String *>(a);
            ASSERT_INT_EQUALS(0, data->compare("TestData"));
        };
        task.cancellationUserdata = &data;

        ASSERT_INT_EQUALS((int)Aws::Crt::Iot::DeviceDefenderV1::ReportTaskStatus::Ready, (int)task.GetStatus());

        ASSERT_INT_EQUALS(AWS_OP_ERR, task.StartTask());
        ASSERT_INT_EQUALS(AWS_ERROR_IOTDEVICE_DEFENDER_UNSUPPORTED_REPORT_FORMAT, task.LastError());

        mqttConnection->Disconnect();
        ASSERT_TRUE(mqttConnection);

        ASSERT_FALSE(mqttClient);
    }

    return AWS_ERROR_SUCCESS;
}

AWS_TEST_CASE(DeviceDefenderFailedTest, s_TestDeviceDefenderFailedTest)