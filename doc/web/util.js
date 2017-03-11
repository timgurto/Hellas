function getID(){
    var url = location.search;
    var params = new Object();
    if (url.indexOf("?") != -1) {
        var str = url.substr(1);
        strs = str.split("&");
        for(var i = 0; i < strs.length; i ++) {
            var splitStr = strs[i].split("=");
            var paramName = splitStr[0];
            var val = splitStr[1];
            if (params.hasOwnProperty(paramName)){
                // Multiple references to the same param; construct array
                if (isArray(params[paramName])){
                    params[paramName].push(val);
                } else {
                    params[paramName] = [params[paramName], val];
                }
            } else {
                params[paramName] = unescape(val);
            }
        }
    }
    if ("id" in params)
        return params.id;
    return "";
}

function findObject(id = getID()){
    return findByID(objects, id);
}

function findItem(id = getID()){
    return findByID(items, id);
}

function findByID(collection, id){
    var i;
    for (i = 0; i < collection.length; ++i){
        var entry = collection[i];
        if (entry.id == id)
            return entry;
    }
    return {};
}

function imageNode(entry){
    return '<img src="images/' + entry.image + '.png"/>';
}

function displayAsSeconds(ms){
    return "" + parseInt(ms) / 1000.0 + "s";
}
