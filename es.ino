#include <LiquidCrystal.h>    //For 16x2 LCD display
#include <TinyGPS++.h>        //To decode the GPS module data
#include <WiFi.h>             //For WiFi conectivity
#include <ESP_Mail_Client.h>  //For E-Mail SMTP server

//Pin Configuration
#define LCD_RS 15  // Register select pin
#define LCD_EN 2   // Enable pin
#define LCD_D4 4   // Data pin 4
#define LCD_D5 5   // Data pin 5
#define LCD_D6 18  // Data pin 6
#define LCD_D7 19  // Data pin 7
#define Ultrasonic_Echo 21
#define Ultrasonic_Trigger 22
#define Engine_pin 13
#define Buzzer_pin 12
#define Switch 23
#define Accl_X_pin A0
#define Accl_Y_pin A3
// GPS_Rx TxD
// GPS_Tx RxD
// RFID_Rx Tx2
// RFID_Tx Rx2
#define my_RFID_String "10004AF701AC"  //Your RFID code
#define Accl_Cal_Sample 50             //Number of samples for Calibration
#define Max_Accl 300                   //Accalarometer threshold value for accident detection

//-----------For WiFi----------------------//
#define WIFI_SSID "Redmi"          //WiFi name
#define WIFI_PASSWORD "redmimder"  //WiFi password

//---------------For Email-------------------//
// The smtp host name e.g. smtp.gmail.com for GMail or smtp.office365.com for Outlook or smtp.mail.yahoo.com
#define SMTP_HOST "smtp.gmail.com"
#define SMTP_PORT 465

// The sign in credentials
#define AUTHOR_EMAIL "myesp32devkitv1@gmail.com"  //Sende E-Mail address
#define AUTHOR_PASSWORD "qano dkcy munu ikdh"     //Sender application key

// Recipient's email
#define RECIPIENT_EMAIL "okok07732@gmail.com"  //Receiver e-Mail address

TinyGPSPlus gps;  // the TinyGPS++ object

SMTPSession smtp;  // Declare the global used SMTPSession object for SMTP transport

LiquidCrystal lcd(LCD_RS, LCD_EN, LCD_D4, LCD_D5, LCD_D6, LCD_D7);  //16x2 LCD oin attache ti library

long Accl_X_Offset;  //X-axis offset value of accarometer
long Accl_Y_Offset;  //Y-axis offset value of accarometer
int Accl_X_value;    //X-axis value of accarometer
int Accl_Y_value;    //Y-axis value of accarometer
unsigned long Accident_time;
double GPS_Latitude;    //Latitude value form GPS at the time of accident
double GPS_Longitude;   //Latitude value form GPS at the time of accident
int Buzzer_status = 1;  //Store the current status of buzzer to prevent the LCD from flickering
void setup() {
  pinMode(Ultrasonic_Echo, INPUT);
  // pinMode(Accl_X_pin, INPUT);
  // pinMode(Accl_Y_pin, INPUT);
  pinMode(Switch, INPUT);
  pinMode(Engine_pin, OUTPUT);
  // pinMode(LCD_RS, OUTPUT);
  // pinMode(LCD_EN, OUTPUT);
  // pinMode(LCD_D4, OUTPUT);
  // pinMode(LCD_D5, OUTPUT);
  // pinMode(LCD_D6, OUTPUT);
  // pinMode(LCD_D7, OUTPUT);
  pinMode(Ultrasonic_Trigger, OUTPUT);
  pinMode(Buzzer_pin, OUTPUT);

  Serial.begin(9600);   //For GPS
  Serial2.begin(9600);  //For RFID

  // for Accelarometer GY-61
  Accelerometer_Setup();  //Calculate Accelarometer Offset

  lcd.print("Welcome to ES");
  delay(3000);

  Scan_Licence();  //Scan and validate the licence

  WIFI_Config();  //Initialize and Connect WiFi

  lcd.clear();
  lcd.print("Engine Started");
  digitalWrite(Engine_pin, HIGH);
  delay(3000);
}


void loop() {
  Check_Acclerometer();  //Check Acclerometer Reading and take action if required
  Check_Distance();      //Check distance and take action if required
}



void Scan_Licence() {
  while (Serial2.available() == 0) {
scan_again:
    lcd.clear();
    delay(500);
    lcd.print("Scan the Licence");
    delay(1000);
  }
  lcd.clear();
  if (Serial2.readString() == my_RFID_String) {
    lcd.print("Successfull");
    delay(2000);
  } else {
    lcd.print("Unsuccessfull");
    delay(2000);
    goto scan_again;
  }
}



void Accelerometer_Setup() {
  Accl_X_Offset = 0;
  Accl_Y_Offset = 0;
  for (int n = 0; n < Accl_Cal_Sample; n++) {
    Accl_X_Offset = Accl_X_Offset + analogRead(Accl_X_pin);
    Accl_Y_Offset = Accl_Y_Offset + analogRead(Accl_Y_pin);
  }
  Accl_X_Offset = Accl_X_Offset / Accl_Cal_Sample;
  Accl_Y_Offset = Accl_Y_Offset / Accl_Cal_Sample;
}


void Check_Acclerometer() {
  Accl_X_value = analogRead(Accl_X_pin);
  Accl_Y_value = analogRead(Accl_Y_pin);
  // Serial.print("   X=");
  // Serial.print(Accl_X_value - Accl_X_Offset);
  // Serial.print("   Y=");
  // Serial.println(Accl_Y_value - Accl_Y_Offset);

  if ((Accl_X_value - Accl_X_Offset) > Max_Accl) {
    Accident_Detected();
  } else if ((Accl_X_value - Accl_X_Offset) < -Max_Accl) {
    Accident_Detected();
  } else if ((Accl_Y_value - Accl_Y_Offset) > Max_Accl) {
    Accident_Detected();
  } else if ((Accl_Y_value - Accl_Y_Offset) < -Max_Accl) {
    Accident_Detected();
  }
}

void Check_Distance() {
  digitalWrite(Ultrasonic_Trigger, HIGH);
  delayMicroseconds(10);
  digitalWrite(Ultrasonic_Trigger, LOW);
  if (pulseIn(Ultrasonic_Echo, HIGH) < 4000) {
    if (Buzzer_status == 0) {
      digitalWrite(Buzzer_pin, HIGH);
      Buzzer_status = 1;
      lcd.clear();
      lcd.print("Drive Slow");
    }
  } else if (Buzzer_status == 1) {
    digitalWrite(Buzzer_pin, LOW);
    Buzzer_status = 0;
    lcd.clear();
    lcd.print("Alright!");
  }
}

void Accident_Detected() {
  digitalWrite(Engine_pin, LOW);
  digitalWrite(Buzzer_pin, HIGH);
  lcd.clear();
  Accident_time = millis();
  while ((millis() - Accident_time) < 10000) {
    if (digitalRead(Switch) == 0) {
      lcd.print("Press the Button");
      delay(1000);
      lcd.clear();
      delay(1000);
    } else {
      goto Switch_pressed;
    }
  }
  Send_aleart();
Switch_pressed:
  {
    digitalWrite(Engine_pin, HIGH);
    digitalWrite(Buzzer_pin, LOW);
  }
}

void Send_aleart() {
  digitalWrite(Buzzer_pin, LOW);
  lcd.clear();
  lcd.write("SOS activated");
  delay(2000);
  Call_GPS();
  Send_EMAIL();
  while (1) {
  }
}

void Call_GPS() {
  while (Serial.available() == 0)
    ;
  if (gps.encode(Serial.read())) {
    if (gps.location.isValid()) {
      GPS_Latitude = gps.location.lat();
      GPS_Longitude = gps.location.lng();
    }
  } else {
    lcd.clear();
    lcd.print("Location Invalid");
  }
}


// Callback function to get the Email sending status
void smtpCallback(SMTP_Status status) {
  // Print the current status
  Serial.println(status.info());

  // Print the sending result
  if (status.success()) {
    // ESP_MAIL_PRINTF used in the examples is for format printing via debug Serial port
    // that works for all supported Arduino platform SDKs e.g. AVR, SAMD, ESP32 and ESP8266.
    // In ESP8266 and ESP32, you can use Serial.printf directly.

    Serial.println("----------------");
    ESP_MAIL_PRINTF("Message sent success: %d\n", status.completedCount());
    ESP_MAIL_PRINTF("Message sent failed: %d\n", status.failedCount());
    Serial.println("----------------\n");

    for (size_t i = 0; i < smtp.sendingResult.size(); i++) {
      // Get the result item
      SMTP_Result result = smtp.sendingResult.getItem(i);

      // In case, ESP32, ESP8266 and SAMD device, the timestamp get from result.timestamp should be valid if
      // your device time was synched with NTP server.
      // Other devices may show invalid timestamp as the device time was not set i.e. it will show Jan 1, 1970.
      // You can call smtp.setSystemTime(xxx) to set device time manually. Where xxx is timestamp (seconds since Jan 1, 1970)

      ESP_MAIL_PRINTF("Message No: %d\n", i + 1);
      ESP_MAIL_PRINTF("Status: %s\n", result.completed ? "success" : "failed");  // my interest
      ESP_MAIL_PRINTF("Date/Time: %s\n", MailClient.Time.getDateTimeString(result.timestamp, "%B %d, %Y %H:%M:%S").c_str());
      ESP_MAIL_PRINTF("Recipient: %s\n", result.recipients.c_str());
      ESP_MAIL_PRINTF("Subject: %s\n", result.subject.c_str());
    }
    Serial.println("----------------\n");

    // You need to clear sending result as the memory usage will grow up.
    smtp.sendingResult.clear();
  }
}


void WIFI_Config() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  lcd.clear();
  lcd.print("Connecting WiFi");
  Serial.print("Connecting WiFi");
  delay(4000);
  while (WiFi.status() != WL_CONNECTED && digitalRead(Switch) == 0) {
    Serial.print(".");
    delay(300);
  }
  lcd.clear();
  lcd.print("WiFi connected");
  delay(4000);
  // Serial.println();
  // Serial.print("Connected with IP: ");
  // Serial.println(WiFi.localIP());
  // Serial.println();
}



void Send_EMAIL() {
  //  Set the network reconnection option
  MailClient.networkReconnect(true);

  // Enable the debug via Serial port
  //  * 0 for no debugging
  //  * 1 for basic level debugging
  //  *
  //  * Debug port can be changed via ESP_MAIL_DEFAULT_DEBUG_PORT in ESP_Mail_FS.h

  smtp.debug(1);

  // Set the callback function to get the sending results
  smtp.callback(smtpCallback);

  // Declare the Session_Config for user defined session credentials
  Session_Config config;

  // Set the session config
  config.server.host_name = SMTP_HOST;
  config.server.port = SMTP_PORT;
  config.login.email = AUTHOR_EMAIL;
  config.login.password = AUTHOR_PASSWORD;
  config.login.user_domain = "";

  // Set the NTP config time
  // For times east of the Prime Meridian use 0-12
  // For times west of the Prime Meridian add 12 to the offset.
  // Ex. American/Denver GMT would be -6. 6 + 12 = 18
  // See https://en.wikipedia.org/wiki/Time_zone for a list of the GMT/UTC timezone offsets

  config.time.ntp_server = F("pool.ntp.org,time.nist.gov");
  config.time.gmt_offset = 3;
  config.time.day_light_offset = 0;

  // Declare the message class
  SMTP_Message message;

  // Set the message headers
  message.sender.name = F("ESP");
  message.sender.email = AUTHOR_EMAIL;
  message.subject = F("[Aleart] Accident Detected");
  message.addRecipient((""), RECIPIENT_EMAIL);

  //Send HTML message
  // String htmlMsg = "<div style=\"color:#2f4468;\"><h1>Hello World!</h1><p>- Sent from ESP board</p></div>";
  // message.html.content = htmlMsg.c_str();
  // message.html.content = htmlMsg.c_str();
  // message.text.charSet = "us-ascii";
  // message.html.transfer_encoding = Content_Transfer_Encoding::enc_7bit;

  //Send raw text message
  String textMsg = "My location: https://www.latlong.net/c/?lat=" + String(GPS_Latitude) + "&long=" + String(GPS_Longitude);
  message.text.content = textMsg.c_str();
  message.text.charSet = "us-ascii";
  message.text.transfer_encoding = Content_Transfer_Encoding::enc_7bit;

  message.priority = esp_mail_smtp_priority::esp_mail_smtp_priority_low;
  message.response.notify = esp_mail_smtp_notify_success | esp_mail_smtp_notify_failure | esp_mail_smtp_notify_delay;

  lcd.clear();

  // Connect to the server
  if (!smtp.connect(&config)) {
    ESP_MAIL_PRINTF("Connection error, Status Code: %d, Error Code: %d, Reason: %s", smtp.statusCode(), smtp.errorCode(), smtp.errorReason().c_str());
    lcd.print("Email not sent");
    return;
  } else {
    lcd.print("Email Sent......");
  }

  if (!smtp.isLoggedIn()) {
    Serial.println("\nNot yet logged in.");
  } else {
    if (smtp.isAuthenticated())
      Serial.println("\nSuccessfully logged in.");
    else
      Serial.println("\nConnected with no Auth.");
  }

  // Start sending Email and close the session
  if (!MailClient.sendMail(&smtp, &message))
    ESP_MAIL_PRINTF("Error, Status Code: %d, Error Code: %d, Reason: %s", smtp.statusCode(), smtp.errorCode(), smtp.errorReason().c_str());
}
