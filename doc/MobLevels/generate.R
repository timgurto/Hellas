library(png)

# Load in CSV
data <- read.csv("data.csv", sep=",")

# Draw map
svg("plot.svg", width=12, height=12, bg=NA)
map <- readPNG("..\\..\\Images\\map.png")
maxCoord = 3000 * 32
plot(NULL, xlim=c(0, maxCoord), ylim=c(maxCoord ,0))
rasterImage(map, 0, 0, maxCoord, maxCoord)

# For each entry
    # Circle radius based on quantity
    # Circle colour based on level
    # Draw circle at x,y
    
