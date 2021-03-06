#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <zmq.hpp>

#ifdef ZMQ_CPP11
#include <thread>
#endif

class mock_monitor_t : public zmq::monitor_t
{
  public:
    MOCK_METHOD2(on_event_connect_delayed, void(const zmq_event_t &, const char *));
    MOCK_METHOD2(on_event_connected, void(const zmq_event_t &, const char *));
};

TEST(monitor, create_destroy)
{
    zmq::monitor_t monitor;
}

TEST(monitor, init_check)
{
    zmq::context_t ctx;
    zmq::socket_t bind_socket(ctx, ZMQ_DEALER);

    bind_socket.bind("tcp://127.0.0.1:*");
    char endpoint[255];
    size_t endpoint_len = sizeof(endpoint);
    bind_socket.getsockopt(ZMQ_LAST_ENDPOINT, &endpoint, &endpoint_len);

    zmq::socket_t connect_socket(ctx, ZMQ_DEALER);

    mock_monitor_t monitor;
    EXPECT_CALL(monitor, on_event_connect_delayed(testing::_, testing::_))
      .Times(testing::AtLeast(1));
    EXPECT_CALL(monitor, on_event_connected(testing::_, testing::_))
      .Times(testing::AtLeast(1));

    monitor.init(connect_socket, "inproc://foo");

    ASSERT_FALSE(monitor.check_event(0));
    connect_socket.connect(endpoint);

    while (monitor.check_event(100)) {
    }
}

#ifdef ZMQ_CPP11
TEST(monitor, init_abort)
{
    zmq::context_t ctx;
    zmq::socket_t bind_socket(ctx, zmq::socket_type::dealer);

    bind_socket.bind("tcp://127.0.0.1:*");
    char endpoint[255];
    size_t endpoint_len = sizeof(endpoint);
    bind_socket.getsockopt(ZMQ_LAST_ENDPOINT, &endpoint, &endpoint_len);

    zmq::socket_t connect_socket(ctx, zmq::socket_type::dealer);

    mock_monitor_t monitor;
    monitor.init(connect_socket, "inproc://foo");
    EXPECT_CALL(monitor, on_event_connect_delayed(testing::_, testing::_))
      .Times(testing::AtLeast(1));
    EXPECT_CALL(monitor, on_event_connected(testing::_, testing::_))
      .Times(testing::AtLeast(1));

    auto thread = std::thread([&monitor] {
        while (monitor.check_event(-1)) {
        }
    });

    connect_socket.connect(endpoint);
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    // TODO instead of sleeping an arbitrary amount of time, we should better
    // wait until the expectations have met. How can this be done with
    // googlemock?

    monitor.abort();
    thread.join();
}
#endif
