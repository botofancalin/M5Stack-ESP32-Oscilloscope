#include <M5Stack.h>

const int LCD_WIDTH = 320;
const int LCD_HEIGHT = 240;
const int SAMPLES = LCD_WIDTH;
const int DOTS_DIV = 30;
const int ad_ch0 = 35; // Analog 35 pin for channel 0
const int ad_ch1 = 36; // Analog 36 pin for channel 1
const long VREF[] = { 250, 500, 1250, 2500, 5000 };
const int MILLIVOL_per_dot[] = { 33, 17, 6, 3, 2 };
const int MODE_ON = 0, MODE_INV = 1, MODE_OFF = 2;
const char *Modes[] = { "NORM", "INV", "OFF" };
const int TRIG_AUTO = 0, TRIG_NORM = 1, TRIG_SCAN = 2;
const char *TRIG_Modes[] = { "Auto", "Norm", "Scan" };
const int TRIG_E_UP = 0, TRIG_E_DN = 1;
#define RATE_MIN 0
#define RATE_MAX 13
const char *Rates[] = { "F1-1", "F1-2", "  F2", " 5ms", "10ms", "20ms", "50ms", "0.1s", "0.2s", "0.5s", "1s", "2s", "5s", "10s" };
#define RANGE_MIN 0
#define RANGE_MAX 4
const char *Ranges[] = { " 1V", "0.5V", "0.2V", "0.1V", "50mV" };
int range0 = RANGE_MIN;
short range1 = RANGE_MIN;
short ch0_mode = MODE_ON;
short ch0_off = 0;
short ch1_mode = MODE_OFF;
short ch1_off = 0;
short rate = 3;
short trig_mode = TRIG_AUTO;
short trig_lv = 40;
short trig_edge = TRIG_E_UP;
short trig_ch = 0;
bool Start = true;
short menu = 19;
short data[4][SAMPLES]; // keep twice of the number of channels to make it a double buffer
short sample = 0;       // index for double buffer
int amplitude = 0;
int amplitudeStep = 5;

TaskHandle_t LedC_Gen;
TaskHandle_t SigmaDeltaGen;

///////////////////////////////////////////////////////////////////////////////////////////////
#define CH1COLOR YELLOW
#define CH2COLOR CYAN
#define GREY 0x7BEF

void DrawText()
{
	M5.Lcd.setTextColor(WHITE);
	M5.Lcd.setTextSize(1);
	M5.Lcd.fillRect(270, 19, 70, 131, BLACK);
	M5.Lcd.fillRect(270, menu, 70, 10, BLUE);
	M5.Lcd.drawString((Start == 0 ? "Stop" : "Run"), 270, 20);
	M5.Lcd.drawString(String(String(Ranges[range0]) + "/DIV"), 270, 30);
	M5.Lcd.drawString(String(String(Ranges[range1]) + "/DIV"), 270, 40);
	M5.Lcd.drawString(String(String(Rates[rate]) + "/DIV"), 270, 50);
	M5.Lcd.drawString(String(Modes[ch0_mode]), 270, 60);
	M5.Lcd.drawString(String(Modes[ch1_mode]), 270, 70);
	M5.Lcd.drawString(String("OFS1:" + String(ch0_off)), 270, 80);
	M5.Lcd.drawString(String("OFS2:" + String(ch1_off)), 270, 90);
	M5.Lcd.drawString(String(trig_ch == 0 ? "T:1" : "T:2"), 270, 100);
	M5.Lcd.drawString(String(TRIG_Modes[trig_mode]), 270, 110);
	M5.Lcd.drawString(String("Tlv:" + String(trig_lv)), 270, 120);
	M5.Lcd.drawString(String((trig_edge == TRIG_E_UP) ? "T:UP" : "T:DN"), 270, 130);
}

void CheckSW()
{
	M5.update();
	if (M5.BtnB.wasPressed())
	{
		(menu < 129) ? (menu += 10) : (menu = 19);
		return;
	}
	else if (M5.BtnA.wasPressed())
	{
		switch (menu)
		{
		case 19:
			Start = !Start;
			break;
		case 29:
			if (range0 > RANGE_MIN)
			{
				range0--;
			}
			break;
		case 39:
			if (range1 > RANGE_MIN)
			{
				range1--;
			}
			break;
		case 49:
			if (rate > RATE_MIN)
			{
				rate--;
			}
			break;
		case 59:
			if (ch0_mode > MODE_ON)
			{
				ch0_mode--;
			}
			else
			{
				ch0_mode = MODE_OFF;
			}
			break;
		case 69:
			if (ch1_mode > MODE_ON)
			{
				ch1_mode--;
			}
			else
			{
				ch1_mode = MODE_OFF;
			}
			break;
		case 79:
			if (ch0_off > -4095)
			{
				ch0_off -= 4096 / VREF[range0];
			}
			break;
		case 89:
			if (ch1_off > -4095)
			{
				ch1_off -= 4096 / VREF[range1];
			}
			break;
		case 99:
			trig_ch = !trig_ch;
			break;
		case 109:
			if (trig_mode > TRIG_AUTO)
			{
				trig_mode--;
			}
			else
			{
				trig_mode = TRIG_SCAN;
			}
			break;
		case 119:
			if (trig_lv > 0)
			{
				trig_lv--;
			}
			break;
		case 129:
			trig_edge = !trig_edge;
			break;
		}
		return;
	}
	if (M5.BtnC.wasPressed())
	{
		switch (menu)
		{
		case 19:
			Start = !Start;
			break;
		case 29:
			if (range0 < RANGE_MAX)
			{
				range0++;
			}
			break;
		case 39:
			if (range1 < RANGE_MAX)
			{
				range1++;
			}
			break;
		case 49:
			if (rate < RATE_MAX)
			{
				rate++;
			}
			break;
		case 59:
			if (ch0_mode < MODE_OFF)
			{
				ch0_mode++;
			}
			else
			{
				ch0_mode = MODE_ON;
			}
			break;
		case 69:
			if (ch1_mode < MODE_OFF)
			{
				ch1_mode++;
			}
			else
			{
				ch1_mode = MODE_ON;
			}
			break;
		case 79:
			if (ch0_off < 4095)
			{
				ch0_off += 4096 / VREF[range0];
			}
			break;
		case 89:
			if (ch1_off < 4095)
			{
				ch1_off += 4096 / VREF[range1];
			}
			break;
		case 99:
			trig_ch = !trig_ch;
			break;
		case 109:
			if (trig_mode < TRIG_SCAN)
			{
				trig_mode++;
			}
			else
			{
				trig_mode = TRIG_AUTO;
			}
			break;
		case 119:
			if (trig_lv < 60)
			{
				trig_lv++;
			}
			break;
		case 129:
			trig_edge = !trig_edge;
			break;
		}
		return;
	}
	else
	{
		return;
	}
	DrawText();
}

void DrawGrid()
{
	for (int x = 0; x <= SAMPLES; x += 2) // Horizontal Line
	{
		for (int y = 0; y <= LCD_HEIGHT; y += DOTS_DIV)
		{
			M5.Lcd.drawPixel(x, y, GREY);
			CheckSW();
		}
		M5.Lcd.drawPixel(x, LCD_HEIGHT - 1, GREY);
	}
	for (int x = 0; x <= SAMPLES; x += DOTS_DIV) // Vertical Line
	{
		for (int y = 0; y <= LCD_HEIGHT; y += 2)
		{
			M5.Lcd.drawPixel(x, y, GREY);
			CheckSW();
		}
	}
	M5.Lcd.drawString("<", 60, 220, 2);
	M5.Lcd.drawString("Menu", 145, 220, 2);
	M5.Lcd.drawString(">", 252, 220, 2);
}


void DrawGrid(int x)
{
	if ((x % 2) == 0)
	{
		for (int y = 0; y <= LCD_HEIGHT; y += DOTS_DIV)
		{
			M5.Lcd.drawPixel(x, y, GREY);
		}
	}
	if ((x % DOTS_DIV) == 0)
	{
		for (int y = 0; y <= LCD_HEIGHT; y += 2)
		{
			M5.Lcd.drawPixel(x, y, GREY);
		}
	}
}

void ClearAndDrawGraph()
{
	int clear = 0;

	if (sample == 0)
	{
		clear = 2;
	}
	for (int x = 0; x < (SAMPLES - 1); x++)
	{
		M5.Lcd.drawLine(x, LCD_HEIGHT - data[clear + 0][x], x + 1, LCD_HEIGHT - data[clear + 0][x + 1], BLACK);
		M5.Lcd.drawLine(x, LCD_HEIGHT - data[clear + 1][x], x + 1, LCD_HEIGHT - data[clear + 1][x + 1], BLACK);
		if (ch0_mode != MODE_OFF)
		{
			M5.Lcd.drawLine(x, LCD_HEIGHT - data[sample + 0][x], x + 1, LCD_HEIGHT - data[sample + 0][x + 1], CH1COLOR);
		}
		if (ch1_mode != MODE_OFF)
		{
			M5.Lcd.drawLine(x, LCD_HEIGHT - data[sample + 1][x], x + 1, LCD_HEIGHT - data[sample + 1][x + 1], CH2COLOR);
		}
		CheckSW();
	}
}

void ClearAndDrawDot(int i)
{
	int clear = 0;

	if (i <= 1)
	{
		return;
	}
	if (sample == 0)
	{
		clear = 2;
	}
	M5.Lcd.drawLine(i - 1, LCD_HEIGHT - data[clear + 0][i - 1], i, LCD_HEIGHT - data[clear + 0][i], BLACK);
	M5.Lcd.drawLine(i - 1, LCD_HEIGHT - data[clear + 1][i - 1], i, LCD_HEIGHT - data[clear + 1][i], BLACK);
	if (ch0_mode != MODE_OFF)
	{
		M5.Lcd.drawLine(i - 1, LCD_HEIGHT - data[sample + 0][i - 1], i, LCD_HEIGHT - data[sample + 0][i], CH1COLOR);
	}
	if (ch1_mode != MODE_OFF)
	{
		M5.Lcd.drawLine(i - 1, LCD_HEIGHT - data[sample + 1][i - 1], i, LCD_HEIGHT - data[sample + 1][i], CH2COLOR);
	}
	DrawGrid(i);
}

void DrawGraph()
{
	for (int x = 0; x < SAMPLES; x++)
	{
		M5.Lcd.drawPixel(x, LCD_HEIGHT - data[sample + 0][x], CH1COLOR);
		M5.Lcd.drawPixel(x, LCD_HEIGHT - data[sample + 1][x], CH2COLOR);
	}
}

void ClearGraph()
{
	int clear = 0;

	if (sample == 0)
	{
		clear = 2;
	}
	for (int x = 0; x < SAMPLES; x++)
	{
		M5.Lcd.drawPixel(x, LCD_HEIGHT - data[clear + 0][x], BLACK);
		M5.Lcd.drawPixel(x, LCD_HEIGHT - data[clear + 1][x], BLACK);
	}
}

inline long adRead(short ch, short mode, int off)
{
	long a = analogRead(ch);
	a = (((a + off) * VREF[(ch == ad_ch0) ? range0 : range1]) / 10000UL) + 30;
	a = ((a >= LCD_HEIGHT) ? LCD_HEIGHT : a);
	if (mode == MODE_INV)
	{
		return LCD_HEIGHT - a;
	}
	return a;
}

void ledcAnalogWrite(uint8_t channel, uint32_t value, uint32_t valueMax = 255)
{
	uint32_t duty = (8191 / valueMax) * min(value, valueMax);
	ledcWrite(channel, duty);
}

// Make a PWM generator task on core 0
// Signal generator pin 2
void LedC_Task(void *parameter)
{
	ledcSetup(0, 50, 13);
	ledcAttachPin(2, 0);

	for (;;)
	{
		ledcAnalogWrite(0, amplitude);
		amplitude = amplitude + amplitudeStep;
		if (amplitude <= 0 || amplitude >= 255)
		{
			amplitudeStep = -amplitudeStep;
		}
		delay(30);
	}
	vTaskDelete(NULL);
}

void SigmaDelta_Task(void *parameter)
{
	sigmaDeltaSetup(0, 312500);
	sigmaDeltaAttachPin(5, 0);
	sigmaDeltaWrite(0, 0);
	for (;;)
	{
		static uint8_t i = 0;
		sigmaDeltaWrite(0, i++);
		delayMicroseconds(50);
	}
	vTaskDelete(NULL);
}

void setup()
{
	M5.begin();
	M5.Lcd.fillScreen(BLACK);
	DrawGrid();
	DrawText();
	M5.Lcd.setBrightness(100);
	dacWrite(25, 0);

	xTaskCreatePinnedToCore(
		LedC_Task,				 /* Task function. */
		"LedC_Task",			 /* name of the task, a name just for humans */
		8192,                    /* Stack size of task */
		NULL,                    /* parameter of the task */
		1,                       /* priority of the task */
		&LedC_Gen,               /* Task handle to keep track of the created task */
		0);                      /*cpu core number where the task is assigned*/

	xTaskCreatePinnedToCore(
		SigmaDelta_Task,		 /* Task function. */
		"SigmaDelta_Task",		 /* name of task, a name just for humans */
		8192,                    /* Stack size of task */
		NULL,                    /* parameter of the task */
		1,                       /* priority of the task */
		&SigmaDeltaGen,          /* Task handle to keep track of the created task */
		0);                      /*cpu core number where the task is assigned*/
}

void loop()
{
	if (trig_mode != TRIG_SCAN)
	{
		unsigned long st = millis();
		short oad;
		if (trig_ch == 0)
		{
			oad = adRead(ad_ch0, ch0_mode, ch0_off);
		}
		else
		{
			oad = adRead(ad_ch1, ch1_mode, ch1_off);
		}
		for (;;)
		{
			short ad;
			if (trig_ch == 0)
			{
				ad = adRead(ad_ch0, ch0_mode, ch0_off);
			}
			else
			{
				ad = adRead(ad_ch1, ch1_mode, ch1_off);
			}

			if (trig_edge == TRIG_E_UP)
			{
				if (ad >= trig_lv && ad > oad)
				{
					break;
				}
			}
			else
			{
				if (ad <= trig_lv && ad < oad)
				{
					break;
				}
			}
			oad = ad;

			CheckSW();
			if (trig_mode == TRIG_SCAN)
			{
				break;
			}
			if (trig_mode == TRIG_AUTO && (millis() - st) > 100)
			{
				break;
			}
		}
	}

	// sample and draw depending on the sampling rate
	if (rate <= 5 && Start)
	{
		if (sample == 0) // change the index for the double buffer
		{
			sample = 2;
		}
		else
		{
			sample = 0;
		}

		if (rate == 0) // full speed, channel 0 only
		{
			for (int i = 0; i < SAMPLES; i++)
			{
				data[sample + 0][i] = adRead(ad_ch0, ch0_mode, ch0_off);
			}
			for (int i = 0; i < SAMPLES; i++)
			{
				data[sample + 1][i] = 0;
			}
		}
		else if (rate == 1) // full speed, channel 1 only
		{
			for (int i = 0; i < SAMPLES; i++)
			{
				data[sample + 1][i] = adRead(ad_ch1, ch1_mode, ch1_off);
			}
			for (int i = 0; i < SAMPLES; i++)
			{
				data[sample + 0][i] = 0;
			}
		}
		else if (rate == 2) // full speed, dual channel
		{
			for (int i = 0; i < SAMPLES; i++)
			{
				data[sample + 0][i] = adRead(ad_ch0, ch0_mode, ch0_off);
				data[sample + 1][i] = adRead(ad_ch1, ch1_mode, ch1_off);
			}
		}
		else if (rate >= 3 && rate <= 5) // .5ms, 1ms or 2ms sampling
		{
			const unsigned long r_[] = { 5000 / DOTS_DIV, 10000 / DOTS_DIV, 20000 / DOTS_DIV };
			unsigned long st = micros();
			unsigned long r = r_[rate - 3];
			for (int i = 0; i < SAMPLES; i++)
			{
				while ((st - micros()) < r)
				{
					;
				}
				st += r;
				data[sample + 0][i] = adRead(ad_ch0, ch0_mode, ch0_off);
				data[sample + 1][i] = adRead(ad_ch1, ch1_mode, ch1_off);
			}
		}
		ClearAndDrawGraph();
		CheckSW();
		DrawGrid();
		DrawText();
	}
	else if (Start)
	{ // 5ms - 500ms sampling
	  // copy currently showing data to another
		if (sample == 0)
		{
			for (int i = 0; i < SAMPLES; i++)
			{
				data[2][i] = data[0][i];
				data[3][i] = data[1][i];
			}
		}
		else
		{
			for (int i = 0; i < SAMPLES; i++)
			{
				data[0][i] = data[2][i];
				data[1][i] = data[3][i];
			}
		}

		const unsigned long r_[] = { 50000 / DOTS_DIV, 100000 / DOTS_DIV, 200000 / DOTS_DIV,
			500000 / DOTS_DIV, 1000000 / DOTS_DIV, 2000000 / DOTS_DIV,
			5000000 / DOTS_DIV, 10000000 / DOTS_DIV };
		unsigned long st = micros();
		for (int i = 0; i < SAMPLES; i++)
		{
			while ((st - micros()) < r_[rate - 6])
			{
				CheckSW();
				if (rate < 6)
				{
					break;
				}
			}
			if (rate < 6) // sampling rate has been changed
			{
				M5.Lcd.fillScreen(BLACK);
				break;
			}
			st += r_[rate - 6];
			if (st - micros() > r_[rate - 6]) // sampling rate has been changed to shorter interval
			{
				st = micros();
			}
			if (!Start)
			{
				i--;
				continue;
			}
			data[sample + 0][i] = adRead(ad_ch0, ch0_mode, ch0_off);
			data[sample + 1][i] = adRead(ad_ch1, ch1_mode, ch1_off);
			ClearAndDrawDot(i);
		}
		DrawGrid();
		DrawText();
	}
	else
	{
		CheckSW();
	}
	M5.update();
}
