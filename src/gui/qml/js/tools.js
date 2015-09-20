//global tools definition

function correctTextFieldIntegerValue(readyComponent, textValue, minValue, maxValue) {
    if(readyComponent.ready) {
        var returnValue = "";
        if(textValue == "" || parseInt(textValue) == 0 || isNaN(parseInt(textValue)) || parseInt(textValue) < parseInt(minValue)) {
            returnValue = parseInt(minValue);
        }
        else if(maxValue && parseInt(textValue) > parseInt(maxValue)) {
            returnValue = parseInt(maxValue);
        }
        else {
            returnValue = parseInt(textValue); //remove leading "0000"
        }
        return returnValue;
    }
}

