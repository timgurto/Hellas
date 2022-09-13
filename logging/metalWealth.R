library("RColorBrewer")
#options(warn=2)
set1 = brewer.pal(9, "Set1")

svg("metalWealth.svg", width=16, height=9)

par(col="#bbbbbb", col.axis="#bbbbbb", col.lab="#bbbbbb", fg="#bbbbbb", bg="black", mar=c(5,4,4,8))

data <- read.csv(
    file="metalWealth.csv",
    header=TRUE,
    sep=",",
    colClasses=c("integer","numeric","numeric","numeric")
)
data$time = as.POSIXct(data$time, origin="1970-01-01")

minValue=min(
    min(data$copper),
    min(data$silver),
    min(data$tin)
)
maxValue=max(
    max(data$copper),
    max(data$silver),
    max(data$tin)
)

COL_TIN="#8778A5"
COL_COPPER="#BA6C62"
COL_SILVER="#C4AAC0"

numEntries = length(data$time)

plot(

    # This specific line should be hidden and not affect axis limits
    x=data$time,
    y=data$tin,
    col="black",
    
    xlab="Date",
    ylab="Total quantity of bars and coins",
    xlim=c(data$time[1],data$time[numEntries]),
    ylim=c(minValue,maxValue),
    log="y"
)
drawOneMetal <- function(metalName, colour){
    for (i in 1:(numEntries-1)){
        segments(
            x0=data$time[i],
            x1=data$time[i+1],
            y0=data[[metalName]][i],
            y1=data[[metalName]][i+1],
            col=colour
        )
    }
    lastVal = data[[metalName]][numEntries]
    axis(
        NULL, side=4, las=1,
        at=lastVal,
        label=paste(lastVal, metalName),
        col.axis=colour
    )
}
drawOneMetal("copper", COL_COPPER)
drawOneMetal("tin", COL_TIN)
drawOneMetal("silver", COL_SILVER)





dev.off()
