<!-- Bargraph.js-->

function BarGraph(ctx) {

  // Private properties and methods
	
  var that = this;
  var startArr;
  var endArr;
  var looping = false;
		
  // Loop method adjusts the height of bar and redraws if neccessary
  var loop = function () {
    ...
  };
		
  // Draw method updates the canvas with the current display
  var draw = function (arr) { 
  var numOfBars = arr.length;
	var barWidth;
	var barHeight;
	var border = 2;
	var ratio;
	var maxBarHeight;
	var gradient;
	var largestValue;
	var graphAreaX = 0;
	var graphAreaY = 0;
	var graphAreaWidth = that.width;
	var graphAreaHeight = that.height;
	var i;
  };
    // Public properties and methods
	
  this.width = 300;
  this.height = 150;	
  this.maxValue;
  this.margin = 5;
  this.colors = ["purple", "red", "green", "yellow"];
  this.curArr = [];
  this.backgroundColor = "#fff";
  this.xAxisLabelArr = [];
  this.yAxisLabelArr = [];
  this.animationInterval = 100;
  this.animationSteps = 10;

  // Update method sets the end bar array and starts the animation
  this.update = function (newArr) {
    ...
  };
}