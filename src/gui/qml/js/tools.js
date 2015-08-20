//global tools definition

function correctTextFieldIntegerValue(readyComponent, textValue, minValue) {
    if(readyComponent.ready) {
        var returnValue = "";
        if(textValue == "" || parseInt(textValue) == 0 || parseInt(textValue) < minValue) {
            returnValue = minValue;
        }
        else {
            returnValue = parseInt(textValue); //remove leading "0000"
        }
        return returnValue;
    }
}

