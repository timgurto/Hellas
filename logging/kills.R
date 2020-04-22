library("RColorBrewer")
#options(warn=2)

svg("kills.svg", width=20, height=7)

par(
    mfrow=c(1,3),
    col="#bbbbbb",
    col.axis="#bbbbbb",
    col.lab="#bbbbbb",
    col.main="#bbbbbb",
    fg="#bbbbbb",
    bg="black")

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
coloursFromPalette = brewer.pal(11, palette)
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

for (class in c("Athlete", "Scholar", "Zealot"))
{
    plot(
        x=NULL,
        xlim=c(0.5, AXIS_MAX+0.5),
        ylim=c(0.5, AXIS_MAX+0.5),
        xlab="Player level",
        ylab="Mob level",
        asp=1,
        bty="n",
        main=class
    )

    for (playerLvl in 1:MAX_PLAYER_LVL){
    for (mobLvl in 1:MAX_MOB_LVL){
        index = (playerLvl-1) * MAX_MOB_LVL + mobLvl
        
        if (class == "Athlete"){
            if (ath_combatCount[index] == 0) {next} # Don't draw anything if no data
            fightTime = ath_combatTimesSum[index] / ath_combatCount[index]
        } else if (class == "Scholar"){
            if (sch_combatCount[index] == 0) {next} # Don't draw anything if no data
            fightTime = sch_combatTimesSum[index] / sch_combatCount[index]
        } else if (class == "Zealot"){
            if (zea_combatCount[index] == 0) {next} # Don't draw anything if no data
            fightTime = zea_combatTimesSum[index] / zea_combatCount[index]
        }
        
        fightTimeNorm = (fightTime - minFightTime) / (maxFightTime - minFightTime)
        fightTimeNorm = 1 - fightTimeNorm
        colour = colours(fightTimeNorm)
        r = colour[1][1]
        g = colour[2][1]
        b = colour[3][1]
        colour = rgb(r, g, b, maxColorValue=255)
        
        brightness = (r+g+b)/3
        if (brightness > 127)
            textColor = "black"
        else
            textColor = "white"
        
        polygon(
            x=c(playerLvl-0.5, playerLvl+0.5, playerLvl+0.5, playerLvl-0.5),
            y=c(mobLvl+0.5, mobLvl+0.5, mobLvl-0.5, mobLvl-0.5),
            col=colour,
            border="black"
        )
        label = round(fightTime, 0)
        
        text(
            x=playerLvl,
            y=mobLvl,
            labels=label,
            col=textColor
        )
        
    }}
}


dev.off()
