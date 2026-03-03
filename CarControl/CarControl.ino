
#define DIR_LEFT 4
#define DIR_RIGHT 7
#define SPEED_RIGHT 6
#define SPEED_LEFT 5

#define FORWARD_LEFT LOW
#define FORWARD_RIGHT LOW

void move(bool left_dir, int left_speed, bool right_dir, int right_speed)
{
  digitalWrite(DIR_LEFT, left_dir);
  digitalWrite(DIR_RIGHT, right_dir);
  analogWrite(SPEED_RIGHT, right_speed);
  analogWrite(SPEED_LEFT, left_speed);
}

void forward(int left_speed, int right_speed)
{
  move(FORWARD_LEFT, left_speed, FORWARD_RIGHT, right_speed);
}

void turn_left(int steepness)
{
  forward(steepness, 255);
}

void turn_right(int steepness)
{
  forward(255, steepness);
}

void rotate_left(int speed)
{
  move(!FORWARD_LEFT, speed, FORWARD_RIGHT, speed);
}

void rotate_right(int speed)
{
  move(FORWARD_LEFT, speed, !FORWARD_RIGHT, speed);
}

void setup()
{
  pinMode(DIR_LEFT, OUTPUT);
  pinMode(DIR_RIGHT, OUTPUT);
  pinMode(SPEED_LEFT, OUTPUT);
  pinMode(SPEED_RIGHT, OUTPUT);

  turn_left(100);
  delay(2000);

  rotate_right(100);
  delay(2000);

  forward(0, 0);
}

void loop()
{
}