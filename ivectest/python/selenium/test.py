# from selenium import webdriver
# from selenium.webdriver.common.by import By
# from selenium.webdriver.chrome.service import Service as ChromeService
# from selenium.webdriver.firefox.service import Service as FirefoxService
# from webdriver_manager.chrome import ChromeDriverManager
# from webdriver_manager.firefox import GeckoDriverManager
# from selenium.webdriver.support.ui import WebDriverWait
# from selenium.webdriver.support import expected_conditions as EC
# import time

# # --- Configuration ---
# INITIAL_URL = "https://payment.ivacbd.com/"
# # THIS IS THE KEY CHANGE: We expect to go here directly after mobile number + proceed
# EXPECTED_URL_AFTER_PROCEED = "https://payment.ivacbd.com/login-auth"
# MOBILE_NUMBER = "01846762961"  # Using the number from your log, replace if different

# # --- Choose your browser ---
# driver = None
# try:
#     print("Attempting to use Chrome...")
#     chrome_options = webdriver.ChromeOptions()
#     chrome_options.add_argument("--no-sandbox")
#     chrome_options.add_argument("--disable-dev-shm-usage")
#     # Keep browser open
#     chrome_options.add_experimental_option("detach", True)
#     driver = webdriver.Chrome(service=ChromeService(ChromeDriverManager().install()), options=chrome_options)
#     print("Chrome WebDriver initialized.")
# except Exception as e_chrome:
#     print(f"Failed to initialize Chrome: {e_chrome}")
#     try:
#         print("Attempting to use Firefox as fallback...")
#         firefox_options = webdriver.FirefoxOptions()
#         driver = webdriver.Firefox(service=FirefoxService(GeckoDriverManager().install()), options=firefox_options)
#         print("Firefox WebDriver initialized.")
#     except Exception as e_firefox:
#         print(f"Failed to initialize Firefox: {e_firefox}")
#         print("Please ensure you have either Google Chrome or Firefox installed.")
#         exit()

# try:
#     print(f"Navigating to: {INITIAL_URL}")
#     driver.get(INITIAL_URL)
#     wait = WebDriverWait(driver, 20) # General wait object

#     # --- Handle the Pop-up ---
#     print("Looking for the pop-up close button...")
#     popup_close_button_locator = (By.XPATH,
#                                   "//button[.//span[@aria-hidden='true' and normalize-space(text())='×'] "
#                                   "or @aria-label='Close' "
#                                   "or contains(@class, 'close') "
#                                   "or contains(text(), '×')]")
#     try:
#         close_button = wait.until(EC.element_to_be_clickable(popup_close_button_locator))
#         print("Pop-up close button found. Clicking it...")
#         try:
#             close_button.click()
#         except Exception: # If direct click is intercepted, try JS click
#             driver.execute_script("arguments[0].click();", close_button)
#         print("Pop-up closed.")
#         time.sleep(1) # Wait for popup to fully disappear
#     except Exception as e_popup:
#         print(f"Could not find or click the pop-up close button (this might be okay): {e_popup}")

#     # --- Fill the mobile number and proceed ---
#     mobile_input_locator = (By.NAME, "mobile_no")
#     print(f"Waiting for mobile input field with locator: {mobile_input_locator}")
#     # Ensure field is visible and then clickable
#     mobile_input_field_visible = wait.until(EC.visibility_of_element_located(mobile_input_locator))
#     mobile_input_field = wait.until(EC.element_to_be_clickable(mobile_input_locator))
#     print("Mobile input field found.")

#     mobile_input_field.clear()
#     mobile_input_field.send_keys(MOBILE_NUMBER)
#     print(f"Entered mobile number: {MOBILE_NUMBER}")

#     proceed_button_locator = (By.ID, "submitButton")
#     print(f"Waiting for proceed button with locator: {proceed_button_locator}")
#     proceed_button = wait.until(EC.element_to_be_clickable(proceed_button_locator))
#     print("Proceed button found.")
#     proceed_button.click()
#     print("Clicked the 'Proceed' button.")

#     # --- Wait for navigation to the EXPECTED AUTHENTICATION page ---
#     print(f"Waiting for navigation to the page: {EXPECTED_URL_AFTER_PROCEED}")
#     try:
#         wait.until(EC.url_to_be(EXPECTED_URL_AFTER_PROCEED))
#         print(f"Successfully navigated to: {driver.current_url}")
#         # You could add checks for elements on the login-auth page here if needed
#         # e.g., wait.until(EC.presence_of_element_located((By.ID, "some_element_on_auth_page")))

#     except Exception as e_nav:
#         print(f"Failed to navigate to {EXPECTED_URL_AFTER_PROCEED} or timed out.")
#         print(f"Current URL is: {driver.current_url}") # This will tell us where it ACTUALLY went
#         print(f"Error during navigation wait: {e_nav}")
#         if driver:
#             driver.save_screenshot("navigation_error.png")
#             print("Screenshot 'navigation_error.png' saved.")

#     print("\nTasks completed. The browser will remain open.")
#     print("You can manually close the browser when you are done.")

# except Exception as e:
#     print(f"An overall error occurred: {e}")
#     if driver:
#         driver.save_screenshot("overall_error_screenshot.png")
#         print("Screenshot saved as overall_error_screenshot.png")

# finally:
#     if driver:
#         print(f"Script finished. Current URL: {driver.current_url}")
#         print("The browser window will stay open. Please close it manually.")
#     # To ensure script doesn't exit immediately if browser doesn't detach
#     # input("Press Enter in this terminal to confirm script end...")



from selenium import webdriver
from selenium.webdriver.common.by import By
from selenium.webdriver.chrome.service import Service as ChromeService
from selenium.webdriver.firefox.service import Service as FirefoxService
from webdriver_manager.chrome import ChromeDriverManager
from webdriver_manager.firefox import GeckoDriverManager
from selenium.webdriver.support.ui import WebDriverWait
from selenium.webdriver.support import expected_conditions as EC
import time

# --- Configuration ---
INITIAL_URL = "https://payment.ivacbd.com/"
EXPECTED_URL_AFTER_PROCEED = "https://payment.ivacbd.com/login-auth"
MOBILE_NUMBER = "01846762961"  # Using the number from your log, replace if different

# --- Choose your browser ---
driver = None
try:
    print("Attempting to use Chrome...")
    chrome_options = webdriver.ChromeOptions()
    chrome_options.add_argument("--no-sandbox")
    chrome_options.add_argument("--disable-dev-shm-usage")
    chrome_options.add_argument("--start-maximized") # Start maximized, sometimes helps with element visibility
    # Keep browser open
    chrome_options.add_experimental_option("detach", True)
    driver = webdriver.Chrome(service=ChromeService(ChromeDriverManager().install()), options=chrome_options)
    print("Chrome WebDriver initialized.")
except Exception as e_chrome:
    print(f"Failed to initialize Chrome: {e_chrome}")
    try:
        print("Attempting to use Firefox as fallback...")
        firefox_options = webdriver.FirefoxOptions()
        driver = webdriver.Firefox(service=FirefoxService(GeckoDriverManager().install()), options=firefox_options)
        print("Firefox WebDriver initialized.")
    except Exception as e_firefox:
        print(f"Failed to initialize Firefox: {e_firefox}")
        print("Please ensure you have either Google Chrome or Firefox installed.")
        exit()

try:
    print(f"Navigating to: {INITIAL_URL}")
    driver.get(INITIAL_URL)
    wait = WebDriverWait(driver, 20) # General wait object

    # --- Handle the Pop-up ---
    print("Looking for the pop-up close button with ID 'emergencyNoticeCloseBtn'...")
    # Using By.ID as it's the most specific and reliable based on your HTML
    popup_close_button_locator = (By.ID, "emergencyNoticeCloseBtn")

    try:
        # Wait for the element to be present first, then clickable
        # Some popups render the element before it's truly interactive
        wait.until(EC.presence_of_element_located(popup_close_button_locator))
        close_button = wait.until(EC.element_to_be_clickable(popup_close_button_locator))
        
        print("Pop-up close button found. Clicking it...")
        # Try direct click first
        try:
            close_button.click()
        except Exception as e_click:
            print(f"Direct click failed ({e_click}), trying JavaScript click...")
            # If direct click is intercepted (e.g., by an overlay or unusual event handling)
            driver.execute_script("arguments[0].click();", close_button)
        
        print("Pop-up closed.")
        # Wait a brief moment for the pop-up to fully animate/disappear
        time.sleep(1.5) # Increased slightly
    except Exception as e_popup:
        print(f"Could not find or click the pop-up close button (ID: emergencyNoticeCloseBtn). This might be okay if it doesn't always appear, or the ID is incorrect/dynamic.")
        print(f"Error details: {e_popup}")
        # If pop-up is critical and not found, you might want to exit or take a screenshot
        # driver.save_screenshot("popup_not_found_error.png")

    # --- Fill the mobile number and proceed ---
    mobile_input_locator = (By.NAME, "mobile_no")
    print(f"Waiting for mobile input field with locator: {mobile_input_locator}")
    # Ensure field is visible and then clickable
    mobile_input_field_visible = wait.until(EC.visibility_of_element_located(mobile_input_locator))
    mobile_input_field = wait.until(EC.element_to_be_clickable(mobile_input_locator))
    print("Mobile input field found.")

    mobile_input_field.clear()
    mobile_input_field.send_keys(MOBILE_NUMBER)
    print(f"Entered mobile number: {MOBILE_NUMBER}")

    proceed_button_locator = (By.ID, "submitButton")
    print(f"Waiting for proceed button with locator: {proceed_button_locator}")
    proceed_button = wait.until(EC.element_to_be_clickable(proceed_button_locator))
    print("Proceed button found.")
    proceed_button.click()
    print("Clicked the 'Proceed' button.")

    # --- Wait for navigation to the EXPECTED AUTHENTICATION page ---
    print(f"Waiting for navigation to the page: {EXPECTED_URL_AFTER_PROCEED}")
    try:
        # It's good to also check if the URL *starts with* if there are potential query params
        wait.until(EC.url_to_be(EXPECTED_URL_AFTER_PROCEED))
        # Alternative: wait.until(EC.url_contains("login-auth")) # If URL can have parameters
        print(f"Successfully navigated to: {driver.current_url}")

    except Exception as e_nav:
        print(f"Failed to navigate to {EXPECTED_URL_AFTER_PROCEED} or timed out.")
        print(f"Current URL is: {driver.current_url}")
        print(f"Error during navigation wait: {e_nav}")
        if driver:
            driver.save_screenshot("navigation_error.png")
            print("Screenshot 'navigation_error.png' saved.")

    print("\nTasks completed. The browser will remain open.")
    print("You can manually close the browser when you are done.")

except Exception as e:
    print(f"An overall error occurred: {e}")
    if driver:
        driver.save_screenshot("overall_error_screenshot.png")
        print("Screenshot saved as overall_error_screenshot.png")

finally:
    if driver:
        print(f"Script finished. Current URL: {driver.current_url}")
        print("The browser window will stay open. Please close it manually.")