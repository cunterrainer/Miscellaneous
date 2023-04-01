#define RED    2
#define YELLOW 0
#define GREEN  4

#define RED_TIME 1000
#define YELLOW_TIME 300

#define On(led) digitalWrite(led, HIGH)
#define Off(led) digitalWrite(led, LOW)

void setup() 
{
  pinMode(RED, OUTPUT);
  pinMode(YELLOW, OUTPUT);
  pinMode(GREEN, OUTPUT);

  Off(RED);
  Off(YELLOW);
  Off(GREEN);
}

void loop() 
{
  On(RED);
  delay(RED_TIME);
  On(YELLOW);
  delay(YELLOW_TIME);
  Off(RED);
  Off(YELLOW);
  On(GREEN);
  delay(RED_TIME);
  Off(GREEN);
  On(YELLOW);
  delay(YELLOW_TIME);
  Off(YELLOW);
}
