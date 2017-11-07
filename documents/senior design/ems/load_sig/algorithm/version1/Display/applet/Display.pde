color light_green = color(128, 255, 0);
color black = color(0, 0, 0);
color white = color(255, 255, 255);
color blue = color(0, 89, 255);
color red = color(255, 0, 0);
color realblue = color(0, 0, 255);
color green = color(0, 255, 0);
color yellow = color(255, 255, 0);
color cyan = color(255, 0, 255);

// Declaring a variable of type PImage
PImage img;
PImage hp;
PImage fan;
PImage tv;
PImage stb;
PImage question;

//load string
String[] output;

PFont fontA;

int border = 30;
int small_border = 5;

boolean mouseclicked = false;

void setup()
{
  //background border
  size(1370, 770);
  background(blue);

  //background
  stroke(light_green);
  fill(light_green);
  rect(border, border, width-2*border, height-2*border);
  
  //title of template: border
//  fill(blue);
//  rect(80, 70, width-160, 100);
  
  //title of template: background
  stroke(white);
  fill(white);
  rect(80+small_border, 70+small_border, width-160-2*small_border, 100-2*small_border);
  
  //Loads a font
  fontA = loadFont("SansSerif.bold-48.vlw");
  textFont(fontA, 40);
  
  //Appliance Box: border
//  fill(blue);
//  rect(80, 240, 450, 330);
  
  //Appliance Box: background
  stroke(white);
  fill(white);
  rect(80+small_border, 240+small_border, width/2-120-2*small_border, 450-2*small_border);
  
  //Image Box: border
//  fill(blue);
//  rect(width/2+80, 240, 320, 330);
  
  //Image Box: background
  stroke(white);
  fill(white);
  rect(width/2+80+small_border, 240+small_border, 520-2*small_border, 450-2*small_border);
  
  
  // Title font
  fill(black);
  text("Device Detection Algorithm", 100, 140);
  
  // Appliances Text
  textFont(fontA, 30);
  text("Vornado Fan", 100, 300);
  text("Sharp TV", 100, 390);
  text("Direct TV Set-Top-Box", 100, 480);
  text("HP Laptop", 100, 570);
  text("Unidentified Appliance", 100, 660);
  text("The Device is a:", width/2+100, 300);
  
  // ellipses, indicating whether or not program "detects" item
  for (int i = 0; i < 5; i = i+1)
  {
    stroke(black);
    fill(white);
    ellipse(width/2-80, 285+i*90, 20, 20);
  }
  
  img = loadImage("calplug_logo.png");
  hp = loadImage("HP_Laptop.jpg");
  fan = loadImage("Vornado_Foot_Fan.jpg");
  tv = loadImage("Sharp_TV.jpg");
  stb = loadImage("Set_Top_Box.jpg");
  question = loadImage("Question_mark.PNG");
  image(img, 950, 90);
  
  noLoop();
}

void draw()
{
  if(mouseclicked)
  {
    if(output[0].equals("0"))
    {
      fill(red);
      ellipse(width/2-80, 285, 15, 15);
      image(fan, width/2+200, 400);
    }
    if(output[0].equals("1"))
    {
      fill(realblue);
      ellipse(width/2-80, 375, 15, 15);
      image(tv, width/2+200, 400);
    }
    if(output[0].equals("2"))
    {
      fill(green);
      ellipse(width/2-80, 465, 15, 15);
      image(stb, width/2+200, 400);
    }
    if(output[0].equals("3"))
    {
      fill(yellow);
      ellipse(width/2-80, 555, 15, 15);
      image(hp, width/2+200, 400);
    }
    if(output[0].equals("4"))
    {
      fill(cyan);
      ellipse(width/2-80, 645, 15, 15);
      image(question, width/2+200, 400);
    }
  }
//  text("Rise Time: " + output[1], width/2+20, 600);
//  text("Normalized Magnitude at 180 Hz: " + output[2], width/2+20, 660);
//  text("Normalized Magnitude at 300 Hz: " + output[3], width/2+20, 720);
}

void mousePressed() 
{
  output = loadStrings("Output.txt");
//  text(output[0], 180, 600);
  

  mouseclicked = true;
  loop();
}



