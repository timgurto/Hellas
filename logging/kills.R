library("RColorBrewer")
#options(warn=2)

svg("kills.svg", width=16, height=9)

par(col="#bbbbbb", col.axis="#bbbbbb", col.lab="#bbbbbb", fg="#bbbbbb", bg="black")

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
combatTimesSum = vector("numeric", VECTOR_SIZE)
combatCount = vector("numeric", VECTOR_SIZE)


plot(
    x=NULL,
    xlim=c(0.5, max(data$killerLevel)+0.5),
    ylim=c(0.5, max(data$npcLevel)+0.5),
    xlab="Player level",
    ylab="Mob level"
)

# Organise data into "2D" arrays
for (i in 1:length(data$npcID)){
    if (data$threatCount[i] != 1) {next}
    if (data$isRanged[i]) {next}
    
    playerLvl = data$killerLevel[i]
    mobLvl = data$npcLevel[i]
    index = (playerLvl-1) * MAX_MOB_LVL + mobLvl
    
    combatTimesSum[index] = combatTimesSum[index] + data$duration[i]
    combatCount[index] = combatCount[index] + 1
}
    
for (playerLvl in 1:MAX_PLAYER_LVL){
for (mobLvl in 1:MAX_MOB_LVL){
    index = (playerLvl-1) * MAX_MOB_LVL + mobLvl
    
    if (combatCount[index] == 0) {next} # Don't draw anything if no data
    
    fightTime = combatTimesSum[index] / combatCount[index]
    text(
        x=playerLvl,
        y=mobLvl,
        labels=fightTime
    )
    
}
}



dev.off()
