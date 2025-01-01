# import os
# import psycopg2
# from dotenv import load_dotenv

# load_dotenv()

# # Database connection parameters from environment variables
# DB_USER = os.getenv("DB_USER")
# DB_PASSWORD = os.getenv("DB_PASSWORD")
# DB_HOST = os.getenv("DB_HOST")
# DB_NAME = os.getenv("DB_NAME")
# DB_PORT = os.getenv("DB_PORT")
# DB_SSL_REJECT_UNAUTHORIZED = os.getenv("DB_SSL_REJECT_UNAUTHORIZED") == "true"

# try:
#    conn = psycopg2.connect(
#       user=DB_USER,
#       password=DB_PASSWORD,
#       host=DB_HOST,
#       database=DB_NAME,
#       port=DB_PORT,
#       sslmode="require" if DB_SSL_REJECT_UNAUTHORIZED else "disable",
#       sslcert="server-ca.pem" if DB_SSL_REJECT_UNAUTHORIZED else None,
#    )
#    print("Database connected successfully.")
#    conn.close()

# except Exception as e:
#    print(f"Error connecting to the database: {e}")



import os
import psycopg2
from dotenv import load_dotenv

load_dotenv()

DB_USER = os.getenv("DB_USER")
DB_PASSWORD = os.getenv("DB_PASSWORD")
DB_HOST = os.getenv("DB_HOST")
DB_NAME = os.getenv("DB_NAME")
DB_PORT = os.getenv("DB_PORT")

try:
    conn = psycopg2.connect(
        user=DB_USER,
        password=DB_PASSWORD,
        host=DB_HOST,
        database=DB_NAME,
        port=DB_PORT,
        # Removed SSL-related parameters for testing
    )
    print("Database connected successfully (no SSL).")
    conn.close()

except Exception as e:
    print(f"Error connecting to the database: {e}")