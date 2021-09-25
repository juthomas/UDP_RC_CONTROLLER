
#include "ESPAsyncWebServer.h"
#include "tripodes.h"

char *APssid;
char *APpassword;

char *local_dns;
char *bluetooth_mac_addr;
int16_t bluetooth_active = 1;

static const int servo1Pin = 33;
static const int servo2Pin = 32;

int16_t servo_1_min = 0;
int16_t servo_1_mid = 90;
int16_t servo_1_max = 180;
int16_t servo_2_min = 0;
int16_t servo_2_mid = 90;
int16_t servo_2_max = 180;

Servo servo1;
Servo servo2;


WiFiUDP Udp;

AsyncWebServer server(80);

bool oscAddressChanged = false;
unsigned int udp_in_port = 49141;

WiFiClass WiFiAP;

const char *wl_status_to_string(int ah)
{
	switch (ah)
	{
	case WL_NO_SHIELD:
		return "WL_NO_SHIELD";
	case WL_IDLE_STATUS:
		return "WL_IDLE_STATUS";
	case WL_NO_SSID_AVAIL:
		return "WL_NO_SSID_AVAIL";
	case WL_SCAN_COMPLETED:
		return "WL_SCAN_COMPLETED";
	case WL_CONNECTED:
		return "WL_CONNECTED";
	case WL_CONNECT_FAILED:
		return "WL_CONNECT_FAILED";
	case WL_CONNECTION_LOST:
		return "WL_CONNECTION_LOST";
	case WL_DISCONNECTED:
		return "WL_DISCONNECTED";
	default:
		return "ERROR NOT VALID WL";
	}
}

const char *eTaskGetState_to_string(int ah)
{
	switch (ah)
	{
	case eRunning:
		return "eRunning";
	case eReady:
		return "eReady";
	case eBlocked:
		return "eBlocked";
	case eSuspended:
		return "eSuspended";
	case eDeleted:
		return "eDeleted";
	default:
		return "ERROR NOT STATE";
	}
}

double fmap(double x, double in_min, double in_max, double out_min, double out_max)
{
	const double dividend = out_max - out_min;
	const double divisor = in_max - in_min;
	const double delta = x - in_min;
	if (divisor == 0)
	{
		log_e("Invalid map input range, min == max");
		return -1; //AVR returns -1, SAM returns 0
	}
	return (delta * dividend + (divisor / 2.0)) / divisor + out_min;
}

String processor(const String &var)
{
	Serial.println(var);
	if (var == "SERVO1MIN")
	{
		char *intStr;
		intStr = (char *)malloc(sizeof(char) * 15);
		intStr = itoa(servo_1_min, intStr, 10);
		String StringPos = String(intStr);
		free(intStr);
		return (StringPos);
	}
	else if (var == "SERVO1MID")
	{
		char *intStr;
		intStr = (char *)malloc(sizeof(char) * 15);
		intStr = itoa(servo_1_mid, intStr, 10);
		String StringPos = String(intStr);
		free(intStr);
		return (StringPos);
	}
	else if (var == "SERVO1MAX")
	{
		char *intStr;
		intStr = (char *)malloc(sizeof(char) * 15);
		intStr = itoa(servo_1_max, intStr, 10);
		String StringPos = String(intStr);
		free(intStr);
		return (StringPos);
	}
	else if (var == "SERVO2MIN")
	{
		char *intStr;
		intStr = (char *)malloc(sizeof(char) * 15);
		intStr = itoa(servo_2_min, intStr, 10);
		String StringPos = String(intStr);
		free(intStr);
		return (StringPos);
	}
	else if (var == "SERVO2MID")
	{
		char *intStr;
		intStr = (char *)malloc(sizeof(char) * 15);
		intStr = itoa(servo_2_mid, intStr, 10);
		String StringPos = String(intStr);
		free(intStr);
		return (StringPos);
	}
	else if (var == "SERVO2MAX")
	{
		char *intStr;
		intStr = (char *)malloc(sizeof(char) * 15);
		intStr = itoa(servo_2_max, intStr, 10);
		String StringPos = String(intStr);
		free(intStr);
		return (StringPos);
	}
	else if (var == "UDPINPORT")
	{
		char *intStr;
		intStr = (char *)malloc(sizeof(char) * 15);
		intStr = itoa(udp_in_port, intStr, 10);
		String StringPos = String(intStr);
		free(intStr);
		return (StringPos);
	}
	else if (var == "BLUETOOTHACTIVE")
	{
		char *intStr;
		intStr = (char *)malloc(sizeof(char) * 15);
		intStr = itoa(bluetooth_active, intStr, 10);
		String StringPos = String(intStr);
		free(intStr);
		return (StringPos);
	}
	else if (var == "LOCALDNS")
	{
		String string_local_dns = String(local_dns);
		return (string_local_dns);
	}
	else if (var == "BLUETOOTHMACADDR")
	{
		String string_bluetooth_mac_addr = String(bluetooth_mac_addr);
		return (string_bluetooth_mac_addr);
	}
	else if (var == "APSSID")
	{
		String string_ssid = String(APssid);
		return (string_ssid);
	}
	else if (var == "APPASSWORD")
	{
		String string_password = String(APpassword);
		return (string_password);
	}
	return String();
}

uint8_t get_octet(char *str, uint8_t n)
{
	size_t i = 0;
	uint8_t n_number = 0;
	uint8_t last_octet = 0;
	while (str[i])
	{
		if (i == 0 || str[i - 1] == '.')
		{
			last_octet = atoi(&str[i]);
		}
		if (str[i] == '.')
		{
			n_number++;
		}
		i++;
		if (n_number == n)
		{
			return (last_octet);
		}
	}
	return (last_octet);
}

bool is_key_matching(char *key, char *file, size_t index)
{
	size_t i = 0;
	while (file[index] == key[i] && file[index] != '\n' && file[index] != '\r' && file[index] && key[i])
	{
		index++;
		i++;
	}
	if (key[i] == '\0')
	{
		return (true);
	}
	return (false);
}

char *get_value_from_csv(char *file, size_t index)
{
	size_t value_size = 0;
	char *value = 0;

	while (file[index + value_size] != '\0' && file[index + value_size] != '\n' && file[index + value_size] != '\r')
	{
		value_size++;
	}
	value = (char *)malloc(sizeof(char) * (value_size + 1));
	value_size = 0;
	while (file[index + value_size] != '\0' && file[index + value_size] != '\n' && file[index + value_size] != '\r')
	{
		value[value_size] = file[index + value_size];
		value_size++;
	}
	value[value_size] = '\0';
	return (value);
}

char *concatenate_csv(char *buffer, size_t beg_index, size_t end_index, char *value)
{
	size_t new_size = strlen(buffer) + strlen(value) - (end_index - beg_index) + 1;
	char *new_buffer = (char *)malloc(sizeof(char) * new_size);
	size_t old_index = 0;
	size_t new_index = 0;

	while (old_index < beg_index)
	{
		new_buffer[new_index] = buffer[old_index];
		old_index++;
		new_index++;
	}

	for (size_t i = 0; value[i] != '\0'; i++)
	{
		new_buffer[new_index] = value[i];
		new_index++;
	}
	old_index = end_index;
	while (buffer[old_index] != '\0')
	{
		new_buffer[new_index] = buffer[old_index];
		old_index++;
		new_index++;
	}
	new_buffer[new_index] = '\0';
	return (new_buffer);
}

void set_data_to_csv(char *key, char *value)
{
	fs::File file = SPIFFS.open("/data.csv", "r");
	if (!file)
	{
		Serial.println("Failed to open file for reading");
		return;
	}
	Serial.printf("Key : %s\n", key);
	Serial.printf("Value : %s\n", value);
	Serial.printf("File Size : %d\n", file.size());
	Serial.printf("Free Heap : %d\n", ESP.getFreeHeap());
	uint8_t *buff;
	size_t read_index = 1;
	size_t buff_size = file.size();
	buff = (uint8_t *)malloc(sizeof(char) * (buff_size + 1));
	if ((read_index = file.read(buff, (buff_size))) > 0)
	{
		buff[read_index] = '\0';
		Serial.printf("%s", buff);
	}
	else
	{
		Serial.printf("Error reading file\n");
		Serial.printf("Buff : %s\n", buff);
	}

	bool get_value = false;
	for (size_t i = 0; i < file.size(); i++)
	{
		if (i == 0 || buff[i - 1] == '\n' || buff[i - 1] == '\r')
		{
			if (is_key_matching(key, (char *)buff, i))
			{
				get_value = true;
			}
		}
		if (buff[i - 1] == ',' && get_value)
		{
			size_t beg_index = i;
			size_t end_index = i;
			while (buff[end_index] != '\0' && buff[end_index] != '\n' && buff[end_index] != '\r')
			{
				end_index++;
			}
			char *new_buff = concatenate_csv((char *)buff, beg_index, end_index, value);
			// file.print(new_buff);
			Serial.print("Beg");
			Serial.print(new_buff);
			Serial.print("End");
			file.close();
			file = SPIFFS.open("/data.csv", "w");
			file.print(new_buff);
			file.close();
			free(buff);
			free(new_buff);
			return;
		}
	}
	free(buff);

	file.close();
}

char *get_data_from_csv(char *key)
{
	fs::File file = SPIFFS.open("/data.csv");
	if (!file)
	{
		Serial.println("Failed to open file for reading");
		return (0);
	}
	Serial.print("File size: ");
	Serial.println(file.size());
	if (file.size() == 0)
	{
		return (0);
	}
	uint8_t *buff;
	size_t read_index = 1;
	buff = (uint8_t *)malloc(sizeof(char) * (file.size() + 1));
	if ((read_index = file.read(buff, (file.size() + 1))) > 0)
	{
		buff[read_index] = '\0';
		Serial.printf("%s", buff);
	}

	bool get_value = false;
	for (size_t i = 0; i < file.size(); i++)
	{
		if (i == 0 || buff[i - 1] == '\n')
		{
			if (is_key_matching(key, (char *)buff, i))
			{
				get_value = true;
			}
		}
		if (buff[i - 1] == ',' && get_value)
		{
			char *value = get_value_from_csv((char *)buff, i);
			free(buff);
			file.close();
			return (value);
		}
	}

	free(buff);
	file.close();
	return (0);

}

void setup_server_for_ap()
{

	server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
			  {
				  if (request->hasParam("servo_1_min"))
				  {
					  uint8_t *buff;
					  buff = (uint8_t *)malloc(sizeof(uint8_t) * 50);
					  request->getParam("servo_1_min")->value().toCharArray((char *)buff, 50);
					  servo_1_min = atoi((char *)buff);
					  set_data_to_csv("servo_1_min", (char *)buff);
					  free(buff);
				  }
				  if (request->hasParam("servo_1_mid"))
				  {
					  uint8_t *buff;
					  buff = (uint8_t *)malloc(sizeof(uint8_t) * 50);
					  request->getParam("servo_1_mid")->value().toCharArray((char *)buff, 50);
					  servo_1_mid = atoi((char *)buff);
					  set_data_to_csv("servo_1_mid", (char *)buff);
					  free(buff);
				  }
				  if (request->hasParam("servo_1_max"))
				  {
					  uint8_t *buff;
					  buff = (uint8_t *)malloc(sizeof(uint8_t) * 50);
					  request->getParam("servo_1_max")->value().toCharArray((char *)buff, 50);
					  servo_1_max = atoi((char *)buff);
					  set_data_to_csv("servo_1_max", (char *)buff);
					  free(buff);
				  }
				  if (request->hasParam("servo_2_min"))
				  {
					  uint8_t *buff;
					  buff = (uint8_t *)malloc(sizeof(uint8_t) * 50);
					  request->getParam("servo_2_min")->value().toCharArray((char *)buff, 50);
					  servo_2_min = atoi((char *)buff);
					  set_data_to_csv("servo_2_min", (char *)buff);
					  free(buff);
				  }
				  if (request->hasParam("servo_2_mid"))
				  {
					  uint8_t *buff;
					  buff = (uint8_t *)malloc(sizeof(uint8_t) * 50);
					  request->getParam("servo_2_mid")->value().toCharArray((char *)buff, 50);
					  servo_2_mid = atoi((char *)buff);
					  set_data_to_csv("servo_2_mid", (char *)buff);
					  free(buff);
				  }
				  if (request->hasParam("servo_2_max"))
				  {
					  uint8_t *buff;
					  buff = (uint8_t *)malloc(sizeof(uint8_t) * 50);
					  request->getParam("servo_2_max")->value().toCharArray((char *)buff, 50);
					  servo_2_max = atoi((char *)buff);
					  set_data_to_csv("servo_2_max", (char *)buff);
					  free(buff);
				  }

				    if (request->hasParam("local_dns"))
				    {
				  	  uint8_t *buff;
				  	  buff = (uint8_t *)malloc(sizeof(uint8_t) * 50);
				  	  request->getParam("local_dns")->value().toCharArray((char *)buff, 50);
				  	  if (local_dns)
				  	  {
				  		  free(local_dns);
				  	  }
				  	  local_dns = strdup((char *)buff);
				  	  set_data_to_csv("local_dns", (char *)buff);
				  	  free(buff);
				    }
				  if (request->hasParam("udp_in_port"))
				  {
					  uint8_t *buff;
					  buff = (uint8_t *)malloc(sizeof(uint8_t) * 50);
					  request->getParam("udp_in_port")->value().toCharArray((char *)buff, 50);
					  udp_in_port = atoi((char *)buff);
					  set_data_to_csv("udp_in_port", (char *)buff);
					  free(buff);
				  }
				  if (request->hasParam("bluetooth_active"))
				  {
					  uint8_t *buff;
					  buff = (uint8_t *)malloc(sizeof(uint8_t) * 50);
					  request->getParam("bluetooth_active")->value().toCharArray((char *)buff, 50);
					  bluetooth_active = atoi((char *)buff);
					  set_data_to_csv("bluetooth_active", (char *)buff);
					  free(buff);
				  }
				  if (request->hasParam("bluetooth_mac_addr"))
				  {
					  uint8_t *buff;
					  buff = (uint8_t *)malloc(sizeof(uint8_t) * 50);
					  request->getParam("bluetooth_mac_addr")->value().toCharArray((char *)buff, 50);
					  if (bluetooth_mac_addr)
					  {
						  free(bluetooth_mac_addr);
					  }
					  bluetooth_mac_addr = strdup((char *)buff);
					  //   PS4.end();
					  //   PS4.begin((char*)bluetooth_mac_addr);
					  set_data_to_csv("bluetooth_mac_addr", (char *)buff);
					  free(buff);
				  }
				  if (request->hasParam("ap_ssid"))
				  {
					  uint8_t *buff;
					  buff = (uint8_t *)malloc(sizeof(uint8_t) * 50);
					  request->getParam("ap_ssid")->value().toCharArray((char *)buff, 50);
					  if (APssid)
					  {
						  free(APssid);
					  }
					  APssid = strdup((char *)buff);
					  set_data_to_csv("ap_ssid", (char *)buff);
					  free(buff);
				  }
				  if (request->hasParam("ap_password"))
				  {
					  uint8_t *buff;
					  buff = (uint8_t *)malloc(sizeof(uint8_t) * 50);
					  request->getParam("ap_password")->value().toCharArray((char *)buff, 50);
					  if (APpassword)
					  {
						  free(APpassword);
					  }
					  APpassword = strdup((char *)buff);
					  set_data_to_csv("ap_password", (char *)buff);
					  free(buff);
				  }

				  request->send(SPIFFS, "/index.html", String(), false, processor);
				  Serial.println("Client Here !");
			  });

	server.begin();
}

void setup_credentials()
{
	char *tmp = 0;

	if (tmp = get_data_from_csv("servo_1_min"))
	{
		servo_1_min = atoi(tmp);
		free(tmp);
	}
	if (tmp = get_data_from_csv("servo_1_mid"))
	{
		servo_1_mid = atoi(tmp);
		free(tmp);
	}
	if (tmp = get_data_from_csv("servo_1_max"))
	{
		servo_1_max = atoi(tmp);
		free(tmp);
	}

	if (tmp = get_data_from_csv("servo_2_min"))
	{
		servo_2_min = atoi(tmp);
		free(tmp);
	}
	if (tmp = get_data_from_csv("servo_2_mid"))
	{
		servo_2_mid = atoi(tmp);
		free(tmp);
	}
	if (tmp = get_data_from_csv("servo_2_max"))
	{
		servo_2_max = atoi(tmp);
		free(tmp);
	}
	if (tmp = get_data_from_csv("udp_in_port"))
	{
		udp_in_port = atoi(tmp);
		free(tmp);
	}

	if (tmp = get_data_from_csv("bluetooth_active"))
	{
		bluetooth_active = atoi(tmp);
		free(tmp);
	}

	if ((local_dns = get_data_from_csv("local_dns")) == 0)
	{
		local_dns = strdup("rc");
	}

	if ((bluetooth_mac_addr = get_data_from_csv("bluetooth_mac_addr")) == 0)
	{
		bluetooth_mac_addr = strdup("01:01:01:01:01:01");
	}

	if ((APssid = get_data_from_csv("ap_ssid")) == 0)
	{
		APssid = strdup("tripodesAP");
	}

	if ((APpassword = get_data_from_csv("ap_password")) == 0)
	{
		APpassword = strdup("44448888");
	}
}

void ap_setup()
{
	WiFi.mode(WIFI_AP);
	WiFi.softAP(APssid, APpassword, 1, 0, 10);
	IPAddress myIP = WiFi.softAPIP();
	Serial.print("AP IP address: ");
	Serial.println(myIP);
	setup_server_for_ap();
	Udp.begin(udp_in_port);

	//Setup DNS
	if (!MDNS.begin(local_dns))
	{
		Serial.println("Error setting up MDNS responder!");
		while (1)
		{
			delay(1000);
		}
	}
	Serial.println("mDNS responder started");

	MDNS.addService("http", "tcp", 80);
	MDNS.addService("http", "udp", udp_in_port); //???

}

void setup()
{
	Serial.begin(115200);
	if (!SPIFFS.begin())
	{
		Serial.println("An Error has occurred while mounting SPIFFS");
	}
	setup_credentials();
	Wire.begin();
	servo1.attach(servo1Pin);
	servo2.attach(servo2Pin);

	servo1.write(servo_1_mid);
	servo2.write(servo_2_mid);

	server.serveStatic("/tripode.ico", SPIFFS, "/tripode.ico");
	server.serveStatic("/main.css", SPIFFS, "/main.css");


	ap_setup();
}



void look_for_udp_message()
{
	String convertedPacket;
	char incomingPacket[255];

	int packetSize = Udp.parsePacket();
	if (packetSize)
	{
		int len = Udp.read(incomingPacket, 255);
		if (len > 0)
		{
			incomingPacket[len] = 0;
		}
		Serial.printf("UDP packet contents: %s\n", incomingPacket);
		convertedPacket = String(incomingPacket);
		Serial.print("UDP Packet :");
		Serial.println(convertedPacket);
		if (convertedPacket.indexOf("TURN") > -1 && convertedPacket.indexOf("ACCEL") > -1)
		{
			int turn = convertedPacket.substring(convertedPacket.indexOf("TURN") + 4).toInt();
			int accel = convertedPacket.substring(convertedPacket.indexOf("ACCEL") + 5).toInt();

			Serial.printf("Turn : %d, Accel : %d \n", turn, accel);
			if (accel < 0)
			{
				servo1.write(map(accel, -100, 0, servo_1_min, servo_1_mid));
			}
			else
			{
				servo1.write(map(accel, 0, 100, servo_1_mid, servo_1_max));
			}
			if (turn < 0)
			{
				servo2.write(map(turn, -100, 0, servo_2_min, servo_2_mid));
			}
			else
			{
				servo2.write(map(turn, 0, 100, servo_2_mid, servo_2_max));
			}
		}
	}
}

void loop()
{
	look_for_udp_message();
	delay(25);
}