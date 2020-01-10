library("RColorBrewer")
#options(warn=2)
set1 = brewer.pal(9, "Set1")

svg("onlinePlayers.svg", width=16, height=9)

par(col="#bbbbbb", col.axis="#bbbbbb", col.lab="#bbbbbb", fg="#bbbbbb", bg="black")

data <- read.csv(
    file="onlinePlayers.csv",
    header=FALSE,
    sep=",",
    colClasses=c("integer","integer")
)
names(data) <- c("time","numPlayers")
as.POSIXct(data$time[1], origin="1970-01-01")
data$time = as.POSIXct(data$time, origin="1970-01-01")

plot(
    x=data$time,
    y=data$numPlayers,
    xlab="Date",
    ylab="Number of online players",
    ylim=c(1,max(data$numPlayers)),
    type="p",
    pch=".",
    col=set1[2]
)

dev.off()
