library("RColorBrewer")

circleSize = 1.75

#svg("backlog.svg", width=8, height=8)
png("backlog.png", width=1000, height=1000, type="cairo", pointsize=13)

jitter_log <- function(vals, scaler=0.05) {
  noise <- rnorm(length(vals), mean=0, sd=vals*scaler)
  vals + noise
}

roiPalette = colorRampPalette(brewer.pal(11, "RdYlGn"))
roiCols = roiPalette(19)

data <- read.csv("backlog.csv", sep=",")
x = jitter_log(data$effort)
y = jitter_log(data$value)

maxEffort = max(data$effort)
maxValue = max(data$value)
x = sapply(x, function(n) min(c(n, maxEffort)))
x = sapply(x, function(n) max(c(n, 1)))
y = sapply(y, function(n) min(c(n, maxValue)))
y = sapply(y, function(n) max(c(n, 1)))

# Categories
cats = sort(unique(data$type))
data$typeID = match(data$type, cats)
cols = brewer.pal(length(cats), "Set1")
data$color = cols[data$typeID]

# ROI
vals = c(1, 2, 3, 5, 8, 13, 21, 34, 55, 89)
data$roi = match(data$value, vals) - match(data$effort, vals)
data

plot(
    NULL, log="xy", xlab="Effort", ylab="Value", axes=FALSE, main="Issue backlog",
    xlim=c(min(x), max(x)), ylim=c(min(y), max(y))
)
axis(1, at=c(1,2,3,5,8,13,21))
axis(2, at=c(1,2,3,5,8,13,21,34,55))
#box()




logHalf <- function(a, b){
    sqrt(a*b)
}




topEdge = logHalf(55,89)

polygon(x=c(1,89,89,1), y=c(1,1,89,89), lty=0, col=roiCols[10])

for (e in 1:8){
    midE = logHalf(vals[e], vals[e+1])
    nextMidE = logHalf(vals[e+1], vals[e+2])
    if (e > 1) {
        for (v in 1:e){
            roi = v-e;
            polygon(
                x=c(midE, nextMidE, nextMidE, midE),
                y=c(vals[v], vals[v+1], vals[v+2], vals[v+1]),
                col=roiCols[10+roi],
                border=roiCols[10+roi]
            )
        }
    }
    roi = -e
    polygon(
        x=c(midE, nextMidE, nextMidE),
        y=c(vals[1], vals[1], vals[2]),
        col=roiCols[10+roi],
        border=roiCols[10+roi]
    )
}

for (v in 1:8){
    midV = logHalf(vals[v], vals[v+1])
    nextMidV = logHalf(vals[v+1], vals[v+2])
    if (v > 1){
        for (e in 1:v){
            roi = v-e;
            polygon(
                x=c(vals[e], vals[e+1], vals[e+2], vals[e+1]),
                y=c(midV, nextMidV, nextMidV, midV),
                col=roiCols[10+roi],
                border=roiCols[10+roi]
            )
        }
    }
    roi = v
    polygon(
        x=c(vals[1], vals[1], vals[2]),
        y=c(midV, nextMidV, nextMidV),
        col=roiCols[10+roi],
        border=roiCols[10+roi]
    )
}


points(x, y, pch=21, bg=data$color, cex=circleSize)
#points(x, y, col=data$color, cex=circleSize, lwd=3)


legend(legend=cats, x="topright", fill=cols)










dev.off()