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

plot(
    x=NULL,
    xlim=c(0.5, max(data$killerLevel)+0.5),
    ylim=c(0.5, max(data$npcLevel)+0.5),
    xlab="Player level",
    ylab="Mob level"
)
for (playerLvl in min(data$killerLevel):max(data$killerLevel)){
for (mobLvl in min(data$npcLevel):max(data$npcLevel)){
    fightTimes = vector("numeric", 0)

    for (i in 1:length(data$npcID)){
        if (data$threatCount[i] != 1) {next}
        if (data$isRanged[i]) {next}
        
        if (data$npcLevel[i] != mobLvl) {next}
        if (data$killerLevel[i] != playerLvl) {next}
        
        fightTimes = c(fightTimes, data$duration[i])
    }
    
    if (length(fightTimes) == 0) {next} # Don't draw anything if no data
    
    fightTime = mean(fightTimes)
    text(
        x=playerLvl,
        y=mobLvl,
        labels=fightTime
    )
}}



dev.off()
