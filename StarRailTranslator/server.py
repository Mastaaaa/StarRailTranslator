# -*- coding: utf-8 -*-
import socket
import sys
import os
import requests
import deepl
from dotenv import load_dotenv

HOST = '127.0.0.1'  
PORT = 65432

#retrive api key
load_dotenv()
api_key = os.getenv("API_KEY")
if not api_key:
    print("Api_key on env not found")
    print("Make sure .env file is inside the folder containing the script")
    exit() 
else:
    print("Api key retrieved!")
try:
    translator = deepl.Translator(api_key)
    translator.get_usage() 
    print("Deepl authentication success")
except deepl.DeepLException as e:
    print(f"Error initializing Deepl: {e}")
    exit()

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.bind((HOST, PORT))
    s.listen()
    
    print(f"Python server listening to {HOST}:{PORT}")
    sys.stdout.flush()

    conn, addr = s.accept()
    
    with conn:
        print(f"Connection stable with {addr}")
        sys.stdout.flush()
        while True:
            data = conn.recv(1024)
            if not data:
                break
            text_to_translate = data.decode('utf-8')
            try:
                text_formatted = text_to_translate.replace("\n", " ");
                result = translator.translate_text(text_formatted, target_lang="IT")
                response_bytes = result.text.encode('utf-8')
                conn.sendall(response_bytes)
            except deepl.DeepLException as e:
                print(f"Error during translation: {e}")
                conn.sendall(f"Api error: {e}".encode('utf-8'))
            sys.stdout.flush()

print("Connection closed.")