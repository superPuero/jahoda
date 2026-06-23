import asyncio
import time
import sys

TARGET_HOST = '127.0.0.1'
TARGET_PORT = 1234
CONNECTIONS = 128   

# --- New Tuning Parameters ---
TEST_DURATION = 30.0  # Seconds to run the continuous test
MESSAGE_DELAY = 0.0   # Seconds to wait between messages per client (0.1 = 10 msgs/sec per client)

async def tcp_client(client_id):
    # Stagger connections slightly so we don't overwhelm the Windows listen backlog
    await asyncio.sleep(client_id * 0.002)
    
    messages_sent = 0
    start_time = time.time()
    
    try:
        # 1. Connect
        reader, writer = await asyncio.open_connection(TARGET_HOST, TARGET_PORT)
        
        # 2. Continuously send payloads until the test duration expires
        while time.time() - start_time < TEST_DURATION:
            message = f"Hello from Python client {client_id} - Msg: {messages_sent}\n"
            writer.write(message.encode())
            await writer.drain()
            
            messages_sent += 1
            
            # Yield control back to the event loop so other clients can send
            await asyncio.sleep(MESSAGE_DELAY)
            
        # 3. Graceful disconnect
        writer.close()
        await writer.wait_closed()
        return True, messages_sent

    except ConnectionRefusedError:
        print(f"[!] Client {client_id} connection refused.")
        return False, 0
    except ConnectionResetError:
        print(f"[!] Client {client_id} connection reset by server.")
        return False, messages_sent
    except Exception as e:
        print(f"[!] Client {client_id} failed: {e}")
        return False, messages_sent

async def main():
    print(f"Spawning {CONNECTIONS} concurrent connections to {TARGET_HOST}:{TARGET_PORT}...")
    print(f"Each client will send a message every {MESSAGE_DELAY} seconds for {TEST_DURATION} seconds.")
    
    test_start = time.time()
    
    # Create and fire off tasks
    tasks = [tcp_client(i) for i in range(CONNECTIONS)]
    results = await asyncio.gather(*tasks)
    
    # Tally the results
    successful_clients = 0
    failed_clients = 0
    total_messages = 0
    
    for success, msg_count in results:
        if success:
            successful_clients += 1
        else:
            failed_clients += 1
        total_messages += msg_count
            
    print("\n--- Test Complete ---")
    print(f"Time elapsed: {time.time() - test_start:.2f} seconds")
    print(f"Successful clients: {successful_clients}")
    print(f"Failed clients: {failed_clients}")
    print(f"Total messages blasted: {total_messages}")
    
    # Calculate rough throughput
    if total_messages > 0:
        throughput = total_messages / TEST_DURATION
        print(f"Throughput: ~{throughput:.2f} messages/second")

if __name__ == "__main__":
    if sys.platform == 'win32':
        asyncio.set_event_loop_policy(asyncio.WindowsProactorEventLoopPolicy())
        
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\nTest cancelled by user.")