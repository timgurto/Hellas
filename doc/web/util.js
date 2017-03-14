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

function displayTimeAsHMS(ms){
    var displayString = "";
    var seconds = parseInt(ms) / 1000.0;
    if (seconds >= 3600){
        var hours = parseInt(seconds / 3600);
        seconds = seconds % 3600;
        displayString += hours + "h";
    }
    if (seconds >= 60){
        var minutes = parseInt(seconds / 60);
        seconds = seconds % 60;
        displayString += minutes + "m";
    }
    if (seconds > 0)
        displayString += seconds + "s"
    return displayString;
}

function objectLink(object){
    var linkHTML =
        '<a href="object.html?id=' + object.id + '">'
            + object.name
            + '<br>' + imageNode(object)
        + '</a>';
    return linkHTML;
}

function itemLink(item){
    var linkHTML = 
        '<a href="item.html?id=' + item.id + '">'
            + imageNode(item)
            + ' ' + item.name
        + '</a>';
    return linkHTML;
}

function tagLink(tag){
    var linkHTML = 
        '<a href="tag.html?id=' + tag + '">'
            + tag
        + '</a>';
    return linkHTML;
}

function compileUnlockListItem(lock){
    console.log(lock);
    var link;
    if (linkPageByType[lock.type] == "item")
        link = itemLink(findItem(lock.sourceID));
    else
        link = objectLink(findObject(lock.sourceID));
    
    var linkAddress = linkPageByType[lock.type] + ".html?id=" + lock.sourceID;
    var listText =
            '<li>' + prefixByType[lock.type]
            + '<a href="' + linkAddress + '">' + link + '</a>'
            + suffixByType[lock.type] +  '</li>';
    return listText;
}

function scalarToPercent(scalar){
    var percentage = Math.round((parseFloat(scalar) - 1) * 100);
    var prefix = percentage < 0 ? '-' : '+';
    return prefix + percentage + '%';
}
    
function sortByName(a, b){
    return a.name.localeCompare(b.name);
}

function isGear(item){
    return "gearSlot" in item;
}

function loadNavBar(){
    $("#navBarGetsLoadedInHere").load("navBar.html");
}

function loadNavBarAndForceHorizontal(){
    $("#navBarGetsLoadedInHere").load("navBar.html", function(){
        $("#navBar").removeClass("col-sm-2");
        $("#navBar").children().removeClass("col-sm-12");
    });
}
