var serverIP = "http://10.148.12.97:8000"
var drawing;
var canvas, stage;
var drawingCanvas;

var toClear =0;
var oldPt;
var oldMidPt;
var title;
var color;
var stroke;
var colors;
var index;
// params for straight line drawing
var straight = 1;
var straightCanvas;
var tempPt;
// params for outputing
var strokes;
var this_stroke;

function gridX(x){
    return Math.ceil(x +20);
}
function gridY(y){
    return 500 - Math.ceil(y+20);
}


function handleDblclick(event){
        drawing = 0;
        console.log("doubleclicked!");
    //this_stroke = this_stroke + "\n";
    //strokes = strokes + (this_stroke.toString());
    //console.log(strokes);
    if (stage.mouseInBounds&&straight) {
        var newPt;

        if (Math.abs(oldPt.x - stage.mouseX) < Math.abs(oldPt.y - stage.mouseY) ){
            newPt = new createjs.Point(oldPt.x, stage.mouseY);
        } else {
            newPt = new createjs.Point(stage.mouseX, oldPt.y);
        }

        if(Math.abs(oldPt.x - newPt.x) + Math.abs(oldPt.y - newPt.y)> 10){
            this_stroke += "X"+gridX(newPt.x)+"Y"+gridY(newPt.y) + "D1*\n";
            straightCanvas.setStrokeStyle(stroke, 'round', 'round').moveTo(oldPt.x, oldPt.y).beginStroke(color).lineTo(newPt.x, newPt.y).endStroke();
            stage.update();
        }
        console.log(this_stroke);
        strokes += this_stroke;

        stage.removeChild(tempCanvas);
        tempGraphics = new createjs.Graphics();
        tempCanvas = new createjs.Shape(tempGraphics);
        stage.update();
    }
    stage.removeEventListener("stagemousemove" , handleMouseMove);
    stage.mouseMoveOutside = false;
    stage.update();
}

function handleMouseDown(event) {
    if (toClear){
        toClear = 0;
        console.log("cleaning!!!!");
        stage.autoClear = true;
        stage.removeAllChildren();
        stage.update();
        stage.autoClear = false;
        
        strokes = "";
        this_stroke = "";
    }
    console.log(""+strokes + this_stroke);
    if (!stage.mouseInBounds) return;
    if (stage.contains(title)) { stage.clear(); stage.removeChild(title); }
    // color = colors[(index++)%colors.length];
    // stroke = Math.random()*30 + 10 | 0;
    color = "#828b20";
    stroke = 10;
    if(drawing ==0){
        console.log("handleMouseDown seting old point");
        oldPt = new createjs.Point(stage.mouseX, stage.mouseY);
        tempPt = oldPt;
        this_stroke = "X"+gridX(stage.mouseX)+"Y"+gridY(stage.mouseY) + "D2*\n";
        // console.log(this_stroke+" start");
        stage.addChild(tempCanvas);
        oldMidPt = oldPt;
    }
    drawing = 1;
    stage.addEventListener("stagemousemove" , handleMouseMove);
}

function handleMouseMove(event) {
    if (straight) {
        // console.log("start"+ oldPt.x + " "+oldPt.y);
        // console.log("now"+ tempPt.x + " "+tempPt.y);
        var old_tempPt = new createjs.Point(tempPt.x, tempPt.y);
        if (Math.abs(oldPt.x - stage.mouseX) < Math.abs(oldPt.y - stage.mouseY) ){
            tempPt = new createjs.Point(oldPt.x, stage.mouseY);
        } else {
            tempPt = new createjs.Point(stage.mouseX, oldPt.y);
        }

        // console.log("start2"+ oldPt.x + " "+oldPt.y);
        tempGraphics.clear().setStrokeStyle(stroke+1, 'round', 'round').moveTo(oldPt.x, oldPt.y).beginStroke("#333").lineTo(old_tempPt.x, old_tempPt.y).endStroke();
        stage.update();

        tempGraphics.clear().setStrokeStyle(stroke, 'round', 'round').moveTo(oldPt.x, oldPt.y).beginStroke(color).lineTo(tempPt.x, tempPt.y).endStroke();
        stage.update();
        return;
    }
    var midPt = new createjs.Point(oldPt.x + stage.mouseX>>1, oldPt.y+stage.mouseY>>1);

    drawingCanvas.graphics.clear().setStrokeStyle(stroke, 'round', 'round').beginStroke(color).moveTo(midPt.x, midPt.y).curveTo(oldPt.x, oldPt.y, oldMidPt.x, oldMidPt.y);

    oldPt.x = stage.mouseX;
    oldPt.y = stage.mouseY;
    if(drawing == 1) 
        {this_stroke = this_stroke + "x"+stage.mouseX+"y"+stage.mouseY + "; ";}
    // console.log("here"+drawing);

    oldMidPt.x = midPt.x;
    oldMidPt.y = midPt.y;

    stage.update();
}

function handleMouseUp(event) {
    //this_stroke = this_stroke + "\n";
    //strokes = strokes + (this_stroke.toString());
    //console.log(strokes);
    if (stage.mouseInBounds&&straight) {
        var newPt;

        if (Math.abs(oldPt.x - stage.mouseX) < Math.abs(oldPt.y - stage.mouseY) ){
            newPt = new createjs.Point(oldPt.x, stage.mouseY);
        } else {
            newPt = new createjs.Point(stage.mouseX, oldPt.y);
        }

        if(Math.abs(oldPt.x - newPt.x) + Math.abs(oldPt.y - newPt.y)> 10){
            this_stroke = this_stroke + "X"+gridX(newPt.x)+"Y"+gridY(newPt.y) + "D1*\n";
            straightCanvas.setStrokeStyle(stroke, 'round', 'round').moveTo(oldPt.x, oldPt.y).beginStroke(color).lineTo(newPt.x, newPt.y).endStroke();
            stage.update();
        }

        // console.log("start"+ oldPt.x + " "+oldPt.y);
        // console.log("end"+ newPt.x + " "+newPt.y);
        stage.removeChild(tempCanvas);
        tempGraphics = new createjs.Graphics();
        tempCanvas = new createjs.Shape(tempGraphics);
        stage.update();


        oldPt = newPt;
        tempPt = oldPt;
        //this_stroke = "X"+gridX(stage.mouseX)+"Y"+gridY(stage.mouseY) + "D2*\n";
        // console.log(this_stroke+" start");
        stage.addChild(tempCanvas);
        oldMidPt = oldPt;
    }
    // stage.removeEventListener("stagemousemove" , handleMouseMove);
    stage.update();
}

function init() {

    if (window.top != window) {
        document.getElementById("header").style.display = "none";
    }
    canvas = document.getElementById("myCanvas");
    index = 0;
    colors = ["#828b20", "#b0ac31", "#cbc53d", "#fad779", "#f9e4ad", "#faf2db", "#563512", "#9b4a0b", "#d36600", "#fe8a00", "#f9a71f"];
    strokes = "";
    drawing = 0;
    // console.log("start"+drawing);
    //check to see if we are running in a browser with touch support
    stage = new createjs.Stage(canvas);
    stage.autoClear = false;
    stage.enableDOMEvents(true);

    createjs.Touch.enable(stage);
    createjs.Ticker.setFPS(24);

    
    tempGraphics = new createjs.Graphics();
    tempCanvas = new createjs.Shape(tempGraphics);
    straightCanvas = new createjs.Graphics();
    drawingCanvas = new createjs.Shape(straightCanvas);

    stage.addEventListener("stagemousedown", handleMouseDown);
    stage.addEventListener("stagemouseup", handleMouseUp);
    canvas.addEventListener("dblclick", handleDblclick);

    title = new createjs.Text("Click and Drag\n    to draw", "36px Arial", "#777777");
    title.x = 100;
    title.y = 200;
    stage.addChild(title);

    stage.addChild(drawingCanvas);
    stage.update();
}

function stop() {}


function clearCanvas() {
    toClear=1;
    strokes = "";
    this_stroke = "";
}


