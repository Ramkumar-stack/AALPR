import subprocess
import requests
import time

# Function to start ngrok tunnel
def start_ngrok(port):
    ngrok_process = subprocess.Popen(['ngrok', 'http', str(port)], stdout=subprocess.PIPE)
    time.sleep(5)  # Wait for ngrok to initialize
    tunnel_url = None

    # Get tunnel URL
    try:
        tunnel_url = requests.get("http://localhost:4040/api/tunnels").json()['tunnels'][0]['public_url']
    except Exception as e:
        print(f"Error getting ngrok tunnel URL: {e}")

    return tunnel_url, ngrok_process

# Function to stop ngrok tunnel
def stop_ngrok(ngrok_process):
    ngrok_process.terminate()

# Start ngrok for two services on different ports
if __name__ == "__main__":
    # Assuming services running on port 8000 and 8001 on two systems
    port1 = 8000  # Service on machine 1
    port2 = 8001  # Service on machine 2
    
    # Start ngrok tunnels on both ports
    tunnel1_url, ngrok_process1 = start_ngrok(port1)
    tunnel2_url, ngrok_process2 = start_ngrok(port2)
    
    if tunnel1_url and tunnel2_url:
        print(f"Access Machine 1 via: {tunnel1_url}")
        print(f"Access Machine 2 via: {tunnel2_url}")
    
    try:
        input("Press Enter to stop ngrok tunnels...")
    finally:
        # Stop both ngrok processes
        stop_ngrok(ngrok_process1)
        stop_ngrok(ngrok_process2)
