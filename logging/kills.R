library("RColorBrewer")
#options(warn=2)

svg("kills.svg", width=16, height=9)

par(col="#bbbbbb", col.axis="#bbbbbb", col.lab="#bbbbbb", fg="#bbbbbb", bg="black")

data <- read.csv(
    file="../kills.log",
    header=TRUE,
    sep=",",
    colClasses=c("character","character","integer","integer","integer","integer","integer")
)
data$duration = 1.0*data$duration/1000
data$isRanged = data$isRanged != 0

classes = sort(unique(data$killingClass))
classColors = brewer.pal(length(classes), "Set1")

add.alpha <- function(col, alpha=1){
  if(missing(col))
    stop("Please provide a vector of colours.")
  apply(sapply(col, col2rgb)/255, 2, 
                     function(x) 
                       rgb(x[1], x[2], x[3], alpha=alpha))  
}

data$duration = data$duration * data$threatCount

plot(
    x=NULL,
    xlim=c(min(data$npcLevel), max(data$npcLevel)),
    ylim=c(min(data$duration), max(data$duration)),
    xlab="NPC level",
    ylab="Fight duration (s)"
)
for (i in 1:length(data$npcID)){
    classIndex = match(data$killingClass[i], classes)
    color = classColors[classIndex]
    
    pch = 1
    if (data$killerLevel[i] != data$npcLevel[i]){
        levelDiff = abs(data$killerLevel[i] - data$npcLevel[i])
        alpha = 1-(levelDiff*0.3)
        alpha = max(alpha, 0.1)
        color = add.alpha(color, alpha)
        
        if (data$killerLevel[i] > data$npcLevel[i])
            pch = '+'
        else
            pch = '-'
    }

    points(
        x=jitter(data$npcLevel[i]),
        y=data$duration[i],
        col=color,
        pch=pch
    )
}
PLUS_GLYPH = 43
MINUS_GLYPH = 45
CIRCLE = 1
legend(
    "topleft",
    legend=c(classes, "Higher-level player", "Lower-level player"),
    pch=c(CIRCLE, CIRCLE, CIRCLE, PLUS_GLYPH, MINUS_GLYPH),
    col=c(classColors, "#bbbbbb", "#bbbbbb")
    #fill=c(classColors, NULL,NULL),
    #border=c("#bbbbbb","#bbbbbb","#bbbbbb",NULL,NULL)
)

dev.off()
