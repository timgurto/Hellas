library(png)
library("RColorBrewer")

# Load in CSV
data <- read.csv("data.csv", sep=",")

# Draw map
svg("plot.svg", width=12, height=12, bg=NA)
par(mar=c(0,0,0,0))
map <- readPNG("..\\..\\Images\\map.png")
maxCoord = 3000 * 32
plot(
    NULL,
    xlim=c(0, maxCoord), ylim=c(maxCoord ,0),
    axes = FALSE,
    xlab="", ylab="",
    xaxs = "i", yaxs = "i"
)
rasterImage(map, 0, maxCoord, maxCoord, 0)

#Generate colour palette
palette = colorRampPalette(brewer.pal(11, "RdYlGn"))
paletteSize = max(data$level) - min(data$level) + 1;
levelColours = palette(paletteSize)

colourForLevel <- function(level){
    levelIndex = level - min(data$level) + 1    
    reversedIndex = paletteSize - levelIndex + 1
    levelColours[reversedIndex]
}

# For each entry
for (i in 1:length(data$level)){
    # Circle radius based on quantity
    circleArea = data$quantity[i]
    rawRadius = sqrt(circleArea / 3.14159)
    radius = rawRadius
    
    # Circle colour based on level
    colour = colourForLevel(data$level[i])
    
    # Draw circle at x,y
    points(data$x[i], data$y[i], pch=21, bg=colour, cex=radius, col="black")
}

dev.off()