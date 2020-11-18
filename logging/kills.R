library("RColorBrewer")
#options(warn=2)

svg("kills.svg", width=16, height=9.25)

par(
    col="#bbbbbb",
    col.axis="#bbbbbb",
    col.lab="#bbbbbb",
    col.main="#bbbbbb",
    fg="#bbbbbb",
    bg="black",
    #mar=c(12.1,4.1,4.1,2.1),
    xpd="NA"
)
options(scipen=5)

layout(matrix(c(1,2,3,4,4,4),2,3,byrow=TRUE))

data <- read.csv(
    file="../kills.log",
    header=TRUE,
    sep=",",
    colClasses=c("character","integer","character","integer","integer","integer","integer")
)
data$duration = 1.0*data$duration/1000
data$isRanged = data$isRanged != 0
data$duration = data$duration * data$threatCount

# 1D vector storing 2D data:
#
#                             Player level
#       1             2             3             4             5
# [ . . . . . ] [ . . . . . ] [ . . . . . ] [ . . . . . ] [ . . . . . ]
#   1 2 3 4 5     1 2 3 4 5     1 2 3 4 5     1 2 3 4 5     1 2 3 4 5
#                               Mob level
#
MAX_PLAYER_LVL = max(data$killerLevel)
MAX_MOB_LVL = max(data$npcLevel)
VECTOR_SIZE = MAX_PLAYER_LVL * MAX_MOB_LVL
ath_combatTimesSum = vector("numeric", VECTOR_SIZE)
ath_combatCount = vector("numeric", VECTOR_SIZE)
sch_combatTimesSum = vector("numeric", VECTOR_SIZE)
sch_combatCount = vector("numeric", VECTOR_SIZE)
zea_combatTimesSum = vector("numeric", VECTOR_SIZE)
zea_combatCount = vector("numeric", VECTOR_SIZE)

# Organise data into "2D" arrays
for (i in 1:length(data$npcID)){
    if (data$threatCount[i] != 1) {next}
    
    playerLvl = data$killerLevel[i]
    mobLvl = data$npcLevel[i]
    
    lvlDiff = playerLvl - mobLvl
    if (lvlDiff < -5) {next}
    if (lvlDiff > 5) {next}
    
    index = (playerLvl-1) * MAX_MOB_LVL + mobLvl
    
    if (data$killingClass[i] == "Athlete"){
        ath_combatTimesSum[index] = ath_combatTimesSum[index] + data$duration[i]
        ath_combatCount[index] = ath_combatCount[index] + 1
    } else if (data$killingClass[i] == "Scholar"){
        sch_combatTimesSum[index] = sch_combatTimesSum[index] + data$duration[i]
        sch_combatCount[index] = sch_combatCount[index] + 1
    } else if (data$killingClass[i] == "Zealot"){
        zea_combatTimesSum[index] = zea_combatTimesSum[index] + data$duration[i]
        zea_combatCount[index] = zea_combatCount[index] + 1
    }
}

AXIS_MAX = max(MAX_PLAYER_LVL, MAX_MOB_LVL)

palette = "Spectral" #"RdYlGn"
paletteSize = 11
coloursFromPalette = brewer.pal(paletteSize, palette)
colours = colorRamp(coloursFromPalette)

# Calculate min and max fight times, so that colours can be placed on a spectrum
minFightTime = NA
maxFightTime = NA    
for (playerLvl in 1:MAX_PLAYER_LVL){
for (mobLvl in 1:MAX_MOB_LVL){
    index = (playerLvl-1) * MAX_MOB_LVL + mobLvl
    
    fightTime = ath_combatTimesSum[index] / ath_combatCount[index]
    if (is.na(minFightTime))
        minFightTime = fightTime
    else if (!is.na(fightTime) && fightTime < minFightTime)
        minFightTime = fightTime
    if (is.na(maxFightTime))
        maxFightTime = fightTime
    else if (!is.na(fightTime) && fightTime > maxFightTime)
        maxFightTime = fightTime
    
    fightTime = sch_combatTimesSum[index] / sch_combatCount[index]
    if (is.na(minFightTime))
        minFightTime = fightTime
    else if (!is.na(fightTime) && fightTime < minFightTime)
        minFightTime = fightTime
    if (is.na(maxFightTime))
        maxFightTime = fightTime
    else if (!is.na(fightTime) && fightTime > maxFightTime)
        maxFightTime = fightTime
    
    fightTime = zea_combatTimesSum[index] / zea_combatCount[index]
    if (is.na(minFightTime))
        minFightTime = fightTime
    else if (!is.na(fightTime) && fightTime < minFightTime)
        minFightTime = fightTime
    if (is.na(maxFightTime))
        maxFightTime = fightTime
    else if (!is.na(fightTime) && fightTime > maxFightTime)
        maxFightTime = fightTime
}}

getColour <- function(normalised){
    colour = colours(normalised)
    r = colour[1][1]
    g = colour[2][1]
    b = colour[3][1]
    return (rgb(r, g, b, maxColorValue=255))
}
getTextColour <- function(normalised){
    colour = colours(fightTimeNorm)
    r = colour[1][1]
    g = colour[2][1]
    b = colour[3][1]
    
    brightness = (r+g+b)/3
    if (brightness > 127)
        return ("black")
    else
        return ("white")
}

set1 = brewer.pal(3, "Set1")
getClassColour <- function(class){
    if (class == "Athlete") return (set1[2])
    if (class == "Scholar") return (set1[1])
    if (class == "Zealot") return (set1[3])
    return ("white")
}

for (class in c("Athlete", "Scholar", "Zealot"))
{
    mainTitle = ""
    if (class == "Scholar") mainTitle = "Average fight durations"
    
    plot(
        x=NULL,
        xlim=c(0.5, AXIS_MAX+0.5),
        ylim=c(0.5, AXIS_MAX+0.5),
        #xlab=paste(class, "level"),
        xlab="",
        ylab="Mob level",
        asp=1,
        bty="n",
        main=mainTitle
    )
    mtext( # x-axis label
        paste(class, "level"),
        side=1,
        col=getClassColour(class),
        line=3,
        cex=0.7
    )

    for (playerLvl in 1:MAX_PLAYER_LVL){
    for (mobLvl in 1:MAX_MOB_LVL){
        index = (playerLvl-1) * MAX_MOB_LVL + mobLvl
        
        if (class == "Athlete"){
            if (ath_combatCount[index] == 0) {next} # Don't draw anything if no data
            qty = ath_combatCount[index]
            fightTime = ath_combatTimesSum[index] / qty
        } else if (class == "Scholar"){
            if (sch_combatCount[index] == 0) {next} # Don't draw anything if no data
            qty = sch_combatCount[index]
            fightTime = sch_combatTimesSum[index] / qty
        } else if (class == "Zealot"){
            if (zea_combatCount[index] == 0) {next} # Don't draw anything if no data
            qty = zea_combatCount[index]
            fightTime = zea_combatTimesSum[index] / qty
        }
        
        fightTimeNorm = (fightTime - minFightTime) / (maxFightTime - minFightTime)
        fightTimeNorm = 1 - fightTimeNorm
        colour = getColour(fightTimeNorm)
        
        enoughData = 5
        if (qty < enoughData){
            alpha = qty / enoughData * 255
            rgbVec = col2rgb(colour)
            colour = rgb(rgbVec[1], rgbVec[2], rgbVec[3], alpha, maxColorValue=255)
        }
        
        polygon(
            x=c(playerLvl-0.5, playerLvl+0.5, playerLvl+0.5, playerLvl-0.5),
            y=c(mobLvl+0.5, mobLvl+0.5, mobLvl-0.5, mobLvl-0.5),
            col=colour,
            border="black"
        )
        label = round(fightTime, 0)
        
        #text(
        #    x=playerLvl,
        #    y=mobLvl,
        #    labels=label,
        #    col=getTextColour(fightTimeNorm)
        #)
        
    }}
    
    if (class == "Scholar"){
        legendL = AXIS_MAX*(-1.5)
        legendR = AXIS_MAX*(2.5)
        legendT = -0.225 * AXIS_MAX
        legendB = -0.275 * AXIS_MAX
        numBars = 1000
        barW = (legendR - legendL) / (numBars)
        for (i in 1:numBars){
            proportion = (i-1) / (numBars-1)
            col = getColour(1-proportion)
            left = legendL + (i-1) * barW
            right = left + barW
            polygon(
                x=c(left, right, right, left),
                y=c(legendT, legendT, legendB, legendB),
                col=col,
                border=col
            )
        }
        shortestTime = ceiling(minFightTime)
        longestTime = floor(maxFightTime)
        for (i in shortestTime:longestTime){
            if (i %% 5 != 0) next;
            proportion = (i - minFightTime) / (maxFightTime - minFightTime)
            text(
                x = legendL + proportion * (legendR - legendL),
                y = (legendT + legendB) / 2,
                labels=paste(i, "s", sep=""),
                col = "black"
            )
        }
    }
}



colConstructions = "#333333"
colRecipes="#666666"
colExploration="#9BAE77"

data <- read.csv(
    file="../levels.log",
    header=TRUE,
    sep=",",
    colClasses=c("character","character","integer","integer")
)
data = data[order(data$name),]
data$hoursPlayed = data$secondsPlayed/3600

dataRecipes = read.csv(
    file="../recipes.log",
    header=TRUE,
    sep=",",
    colClasses=c("character","integer","integer")
)
dataRecipes = dataRecipes[order(dataRecipes$name),]
dataRecipes$hoursPlayed = dataRecipes$secondsPlayed/3600
minRecipes = min(dataRecipes$recipesKnown)
maxRecipes = max(dataRecipes$recipesKnown)

dataConstructions = read.csv(
    file="../constructions.log",
    header=TRUE,
    sep=",",
    colClasses=c("character","integer","integer")
)
dataConstructions = dataConstructions[order(dataConstructions$name),]
dataConstructions$hoursPlayed = dataConstructions$secondsPlayed/3600
minConstructions = min(dataConstructions$constructionsKnown)
maxConstructions = max(dataConstructions$constructionsKnown)

dataExploration = read.csv(
    file="../exploration.log",
    header=TRUE,
    sep=",",
    colClasses=c("character","integer","integer")
)
dataExploration = dataExploration[order(dataExploration$name),]
dataExploration$secondsPlayed = replace(dataExploration$secondsPlayed, dataExploration$secondsPlayed==0, 1)
dataExploration$hoursPlayed = dataExploration$secondsPlayed/3600
minExplorations = min(dataExploration$chunksExplored)
maxExplorations = max(dataExploration$chunksExplored)

minTime = min(min(data$hoursPlayed))#, min(dataRecipes$hoursPlayed), min(dataConstructions$hoursPlayed), min(dataExploration$hoursPlayed)))
maxTime = max(max(data$hoursPlayed))#, max(dataRecipes$hoursPlayed), max(dataConstructions$hoursPlayed), max(dataExploration$hoursPlayed)))
minTime

plot(
    x=NULL,
    bty="n",

    xlab="Time played",
    xlim=c(minTime, maxTime),
    log="xy",
    xaxt = "n",
    
    ylab="Level reached",
    ylim=c(2, max(data$level))
)
axis(
    side=1,
    at=c(0.000333, 0.00333,
        0.0333, 0.05, 0.0833,
        0.167, 0.333, 0.5,
        1, 2, 3, 5, 8, 12,
        24, 48, 72, 120#, 192
    ),
    lab=c("1.2s", "12s",
        "2m", "3m", "5m",
        "10m", "20m", "30m",
        "1h", "2h", "3h", "5h", "8h", "12h",
        "1d", "2d", "3d", "5d"#, "8d"
    )
)

lastLevelSeen = 1
timeAtLastLevel = 0
for (i in 1:length(data$name)){
    if (data$level[i] == 2){
        lastLevelSeen = 1
        timeAtLastLevel = 0
    }

    col = getClassColour(data$class[i])
    
    thisLevel = data$level[i]
    thisTime = data$hoursPlayed[i]
    
    if (thisLevel - lastLevelSeen != 1){
        lastLevelSeen = thisLevel
        timeAtLastLevel = thisTime
        next
    }
    
    segments(
        x0 = timeAtLastLevel,
        x1 = thisTime,
        y0 = lastLevelSeen,
        y1 = thisLevel,
        
        col=col
    )
    
    lastLevelSeen = thisLevel
    timeAtLastLevel = thisTime
}

#lastRecipeSeen = 0
#timeAtLastRecipe = 0
#for (i in 1:length(dataRecipes$name)){
#    if (dataRecipes$recipesKnown[i] == 1){
#        lastRecipeSeen = 0
#        timeAtLastRecipe = 0
#    }
#    
#    thisRecipe = dataRecipes$recipesKnown[i]
#    thisTime = dataRecipes$hoursPlayed[i]
#    
#    if ( lastRecipeSeen == 0){
#        lastRecipeSeen = thisRecipe
#        timeAtLastRecipe = thisTime
#        next
#    }
#    
#    segments(
#        x0 = timeAtLastRecipe,
#        x1 = thisTime,
#        y0 = ((lastRecipeSeen-1)/(maxRecipes-1)) * (max(data$level)-2)+2,
#        y1 = ((thisRecipe-1)/(maxRecipes-1)) * (max(data$level)-2)+2,
#        
#        col=colRecipes
#    )
#    
#    lastRecipeSeen = thisRecipe
#    timeAtLastRecipe = thisTime
#}
#
#lastConstructionSeen = 0
#timeAtLastConstruction = 0
#for (i in 1:length(dataConstructions$name)){
#    if (dataConstructions$constructionsKnown[i] == 1){
#        lastConstructionSeen = 0
#        timeAtLastConstruction = 0
#    }
#    
#    thisConstruction = dataConstructions$constructionsKnown[i]
#    thisTime = dataConstructions$hoursPlayed[i]
#    
#    if (lastConstructionSeen == 0){
#        lastConstructionSeen = thisConstruction
#        timeAtLastConstruction = thisTime
#        next
#    }
#    
#    segments(
#        x0 = timeAtLastConstruction,
#        x1 = thisTime,
#        y0 = ((lastConstructionSeen-1)/(maxConstructions-1)) * (max(data$level)-2)+2,
#        y1 = ((thisConstruction-1)/(maxConstructions-1)) * (max(data$level)-2)+2,
#        
#        col=colConstructions
#    )
#    
#    lastConstructionSeen = thisConstruction
#    timeAtLastConstruction = thisTime
#}
#
#lastChunkSeen = 0
#timeAtLastChunk = 0
#for (i in 1:length(dataExploration$name)){
#    if (dataExploration$chunksExplored[i] == 9){
#        lastChunkSeen = 0
#        timeAtLastChunk = 0
#    }
#    
#    thisChunk = dataExploration$chunksExplored[i]
#    thisTime = dataExploration$hoursPlayed[i]
#    
#    if (lastChunkSeen == 0){
#        lastChunkSeen = thisChunk
#        timeAtLastChunk = thisTime
#        next
#    }
#    
#    if (thisTime == 0){
#        next
#    }
#    
#    segments(
#        x0 = timeAtLastChunk,
#        x1 = thisTime,
#        y0 = ((lastChunkSeen-minExplorations)/(maxExplorations-minExplorations)) * (max(data$level)-2)+2,
#        y1 = ((thisChunk-minExplorations)/(maxExplorations-minExplorations)) * (max(data$level)-2)+2,
#        
#        col=colExploration
#    )
#    
#    lastChunkSeen = thisChunk
#    timeAtLastChunk = thisTime
#}

legend(
    "bottomright",
    legend=c(
        "Athlete level",
        "Scholar level",
        "Zealot level"#,
        #"Recipes",
        #"Constructions",
        #"Exploration"
    ),
    fill=c(
        set1[2],
        set1[1],
        set1[3]#,
        #colRecipes,
        #colConstructions,
        #colExploration
    )
)

dev.off()
