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

classes = unique(data$killingClass)
classColors = brewer.pal(length(classes), "Set1")

add.alpha <- function(col, alpha=1){
  if(missing(col))
    stop("Please provide a vector of colours.")
  apply(sapply(col, col2rgb)/255, 2, 
                     function(x) 
                       rgb(x[1], x[2], x[3], alpha=alpha))  
}

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
    
    if (data$killerLevel[i] != data$npcLevel[i])
        color = add.alpha(color, 0.2)

    points(
        x=jitter(data$npcLevel[i]),
        y=data$duration[i],
        col=color
    )
}
legend(
    "bottomright",
    #legend=(paste(classes, "killer")),
    legend=classes,
    fill=classColors
)

dev.off()
