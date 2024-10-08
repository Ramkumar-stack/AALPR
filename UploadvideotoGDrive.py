import os
import pickle
from google.oauth2.credentials import Credentials
from google_auth_oauthlib.flow import InstalledAppFlow  #pip install --upgrade google-api-python-client google-auth-httplib2 google-auth-oauthlib
import time
from google.auth.transport.requests import Request
from googleapiclient.discovery import build
from googleapiclient.http import MediaFileUpload

# Define the scope - we'll need Google Drive file access
SCOPES = ['https://www.googleapis.com/auth/drive.file']

# Function to authenticate and get Google Drive service
def authenticate_google_drive():
    """Authenticates the user and returns the Google Drive service."""
    creds = None
    # Check if token.pickle already exists (stored access/refresh tokens)
    if os.path.exists('token.pickle'):
        with open('token.pickle', 'rb') as token:
            creds = pickle.load(token)
    # If no valid credentials are available, initiate authentication
    if not creds or not creds.valid:
        if creds and creds.expired and creds.refresh_token:
            creds.refresh(Request())
        else:
            # Use the client secrets JSON to authenticate
            flow = InstalledAppFlow.from_client_secrets_file(
                'D:/Ram/learnn/personnalproejcts/Creds/gooDriveOath/cred.json', SCOPES)
            creds = flow.run_local_server(port=0)
           # Save the access/refresh token for later use
        with open('token.pickle', 'wb') as token:
            pickle.dump(creds, token)

    # Create a service object for interacting with Google Drive API
    service = build('drive', 'v3', credentials=creds)
    return service

# Function to upload a file to Google Drive
def upload_file_to_drive(file_path,folder_id, drive_service):
    """Uploads a file to Google Drive."""
    # Extract file name from the path
    file_name = os.path.basename(file_path)
    start_time = time.time()
    # Upload the file
    file_metadata = {'name': file_name, 'parents': [folder_id] }  # Name to show in Google Drive
    media = MediaFileUpload(file_path, resumable=True)
    
    # Create and upload the file
    uploaded_file = drive_service.files().create(
        body=file_metadata,
        media_body=media,
        fields='id'  # We'll just retrieve the file ID
    ).execute()
    end_time = time.time()
    time_taken = (end_time - start_time) * 1000

    print(f"File '{file_name}' uploaded successfully. File ID: {uploaded_file.get('id')}")
    print(f"Time taken to upload: {time_taken:.2f} milliseconds")

# Function to get all .mp4 files from the SSD directory
def get_all_files(directory):
    """Returns a list of all .mp4 files in the directory."""
    return [os.path.join(directory, f) for f in os.listdir(directory) if f.endswith('.mp4')]

# Function to delete files after upload
def delete_files(files):
    """Deletes the files after successful upload."""
    for file in files:
        try:
            os.remove(file)
            print(f"Deleted file: {file}")
        except OSError as e:
            print(f"Error: {file} : {e.strerror}")

# Main function to execute the file upload
def main():
    # Path to the file you want to upload (modify this as needed)
    ssd_directory = "D:/Videos" 
   #file_path = "D:/oops.txt"  # Replace with the file you want to upload
    folder_id="1HRuwW0GmARvBed9gwSi-cE_RdaRe5VEh"
    #https://drive.google.com/drive/folders/1HRuwW0GmARvBed9gwSi-cE_RdaRe5VEh?usp=drive_link"
    
    # Authenticate and get the Google Drive service
    drive_service = authenticate_google_drive()
    
    # Upload the file
    #upload_file_to_drive(file_path,folder_id, drive_service)

    while True:
        # Get all .mp4 files from the SSD directory
        all_files = get_all_files(ssd_directory)
        
        if all_files:
            # Loop through and upload each file
            for file in all_files:
                print(f"Uploading file: {file}")
                upload_file_to_drive(file, folder_id, drive_service)
            
            # After successful upload, delete all uploaded files
            delete_files(all_files)
        else:
            print(f"No .mp4 files found in directory {ssd_directory}")
        
        # Wait for 30 minutes before checking again
        time.sleep(30 * 60)  # 30 minutes = 1800 seconds

# Run the main function
if __name__ == '__main__':
    main()
