#!/usr/bin/env python3
"""
Simple MSSP test script for FieryMUD.
Tests telnet MSSP option negotiation by sending DO MSSP option.
"""

import socket
import sys
import time

def test_mssp():
    # Telnet constants
    IAC = 255
    DO = 253
    DONT = 254
    WILL = 251
    WONT = 252
    SB = 250
    SE = 240
    MSSP_OPTION = 70
    
    try:
        # Connect to the MUD server
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(10)
        sock.connect(('localhost', 4000))
        print("Connected to FieryMUD on localhost:4000")
        
        # Wait a moment for server to be ready
        time.sleep(0.5)
        
        # Send IAC DO MSSP to request MSSP data
        request = bytes([IAC, DO, MSSP_OPTION])
        sock.send(request)
        print(f"Sent MSSP request: {list(request)} (IAC DO {MSSP_OPTION})")
        
        # Read response data in chunks - MSSP might come after other data
        all_data = b''
        start_time = time.time()
        while time.time() - start_time < 3:  # Wait up to 3 seconds
            try:
                sock.settimeout(0.1)  # Short timeout for each receive
                chunk = sock.recv(1024)
                if chunk:
                    all_data += chunk
                    print(f"Received chunk: {len(chunk)} bytes")
                else:
                    break
            except socket.timeout:
                continue
        
        data = all_data
        print(f"Total received: {len(data)} bytes")
        
        if data:
            # Print raw bytes for debugging
            print("Raw data:", list(data[:100]))  # Show first 100 bytes
            
            # Look for MSSP data
            if len(data) >= 3:
                # Check for IAC SB MSSP_OPTION sequence
                mssp_start = -1
                for i in range(len(data) - 2):
                    if data[i] == IAC and data[i+1] == SB and data[i+2] == MSSP_OPTION:
                        mssp_start = i
                        break
                
                if mssp_start >= 0:
                    print(f"Found MSSP data starting at byte {mssp_start}")
                    
                    # Find end of MSSP subnegotiation (IAC SE)
                    mssp_end = -1
                    for i in range(mssp_start + 3, len(data) - 1):
                        if data[i] == IAC and data[i+1] == SE:
                            mssp_end = i + 2
                            break
                    
                    if mssp_end > 0:
                        mssp_data = data[mssp_start:mssp_end]
                        print(f"MSSP subnegotiation data: {len(mssp_data)} bytes")
                        
                        # Parse MSSP variables
                        i = 3  # Skip IAC SB MSSP_OPTION
                        variables = {}
                        
                        while i < len(mssp_data) - 2:  # Stop before IAC SE
                            if mssp_data[i] == 1:  # MSSP_VAR
                                i += 1
                                var_start = i
                                # Find end of variable name
                                while i < len(mssp_data) and mssp_data[i] != 2:
                                    i += 1
                                if i < len(mssp_data):
                                    var_name = mssp_data[var_start:i].decode('utf-8', errors='ignore')
                                    
                                    if mssp_data[i] == 2:  # MSSP_VAL
                                        i += 1
                                        val_start = i
                                        # Find end of value
                                        while i < len(mssp_data) and mssp_data[i] not in [1, IAC]:
                                            i += 1
                                        val = mssp_data[val_start:i].decode('utf-8', errors='ignore')
                                        variables[var_name] = val
                                        print(f"  {var_name}: {val}")
                            else:
                                i += 1
                        
                        print(f"\nParsed {len(variables)} MSSP variables successfully!")
                        
                        # Check key server info
                        key_vars = ['NAME', 'VERSION', 'CODEBASE', 'PORT', 'GMCP', 'MSSP', 'TLS', 'PLAYERS']
                        print("\nKey server information:")
                        for var in key_vars:
                            if var in variables:
                                print(f"  {var}: {variables[var]}")
                    else:
                        print("MSSP data found but no IAC SE terminator")
                else:
                    print("No MSSP subnegotiation found in response")
                    # Check if it's regular telnet data
                    printable = ''.join(chr(b) if 32 <= b <= 126 else '.' for b in data[:200])
                    print(f"Response content (first 200 chars): {printable}")
            else:
                print("Response too short")
        else:
            print("No response received")
        
        sock.close()
        return True
        
    except Exception as e:
        print(f"Error testing MSSP: {e}")
        return False

if __name__ == "__main__":
    print("Testing MSSP functionality on FieryMUD...")
    success = test_mssp()
    sys.exit(0 if success else 1)