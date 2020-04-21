library(shape)
library("RColorBrewer")

#svg("npc-stats.svg", width=5, height=5)
png("npc-stats.png", width=800, height=800,pointsize=20)


id <- c(
    "sheepGuardian"
    ,"pig1"
    ,"goat5"
    ,"goat12"
    ,"goat15"
    ,"goat16"
    ,"tortoise5"
    ,"tortoise11"
    ,"tortoise19"
    ,"lizard6"
    ,"lizard12"
    ,"lizard15"
    ,"lizard18"
    ,"snake8"
    ,"snake16"
    ,"boar10"
    ,"boar12"
    ,"boar18"
    ,"waterNymph"
    ,"airNymph"
    ,"earthNymph"
    ,"fireNymph"
    ,"meleeMan6"
    ,"meleeMan7"
    ,"meleeMan8"
    ,"meleeMan13"
    ,"meleeMan14"
    ,"meleeMan15"
    ,"meleeMan16"
    ,"meleeMan17"
    ,"rangedMan11"
    ,"rangedMan13"
    ,"rangedMan18"
    ,"rangedMan20"
    ,"dog6"
    ,"athleteQuestTortoise"
    ,"tutPig"
    ,"tutGoat"
    ,"tutBoar"
    ,"tutSnake"
    ,"tutTortoise"
    ,"tutLizard"
)

isRanged <- c(
    FALSE
    ,FALSE
    ,FALSE
    ,FALSE
    ,FALSE
    ,FALSE
    ,FALSE
    ,FALSE
    ,FALSE
    ,FALSE
    ,FALSE
    ,FALSE
    ,FALSE
    ,TRUE
    ,TRUE
    ,FALSE
    ,FALSE
    ,FALSE
    ,TRUE
    ,TRUE
    ,FALSE
    ,TRUE
    ,FALSE
    ,FALSE
    ,FALSE
    ,FALSE
    ,FALSE
    ,FALSE
    ,FALSE
    ,FALSE
    ,TRUE
    ,TRUE
    ,TRUE
    ,TRUE
    ,FALSE
    ,FALSE
    ,FALSE
    ,FALSE
    ,FALSE
    ,TRUE
    ,FALSE
    ,FALSE
)

healthBefore <- c(
    48
    ,21
    ,25
    ,37
    ,43
    ,46
    ,23
    ,27
    ,52
    ,23
    ,35
    ,42
    ,51
    ,30
    ,47
    ,33
    ,37
    ,50
    ,33
    ,36
    ,49
    ,57
    ,26
    ,28
    ,29
    ,40
    ,41
    ,43
    ,46
    ,48
    ,36
    ,39
    ,51
    ,59
    ,28
    ,30
    ,20
    ,19
    ,19
    ,14
    ,18
    ,20
)

dpsBefore <- c(
    4.40
    ,1.45
    ,2.18
    ,3.27
    ,4.00
    ,4.00
    ,1.14
    ,1.43
    ,1.71
    ,1.60
    ,2.40
    ,2.80
    ,3.20
    ,1.85
    ,2.77
    ,3.00
    ,3.33
    ,4.67
    ,1.78
    ,2.29
    ,5.00
    ,3.50
    ,2.40
    ,2.40
    ,2.80
    ,3.20
    ,3.60
    ,4.00
    ,4.00
    ,4.40
    ,2.00
    ,2.40
    ,3.20
    ,3.20
    ,2.40
    ,2.29
    ,1.66
    ,2.16
    ,2.52
    ,1.95
    ,2.14
    ,2.17
)

healthAfter <- c(
    71
    ,18
    ,25
    ,47
    ,61
    ,65
    ,26
    ,43
    ,83
    ,24
    ,40
    ,53
    ,69
    ,34
    ,66
    ,39
    ,47
    ,77
    ,38
    ,49
    ,76
    ,91
    ,28
    ,30
    ,34
    ,51
    ,56
    ,61
    ,64
    ,71
    ,73
    ,52
    ,76
    ,89
    ,28
    ,33
    ,18
    ,20
    ,21
    ,23
    ,26
    ,23
)

dpsAfter <- c(
    6.80
    ,1.64
    ,2.55
    ,4.56
    ,5.66
    ,6.42
    ,1.43
    ,1.71
    ,4.72
    ,2.38
    ,4.58
    ,5.88
    ,7.60
    ,2.29
    ,4.44
    ,3.87
    ,4.53
    ,7.46
    ,2.38
    ,3.37
    ,6.76
    ,6.22
    ,2.80
    ,3.02
    ,3.20
    ,5.00
    ,5.31
    ,5.71
    ,6.53
    ,6.80
    ,3.02
    ,3.46
    ,5.37
    ,6.40
    ,2.76
    ,3.78
    ,1.66
    ,1.85
    ,2.14
    ,1.67
    ,1.43
    ,2.53
)


palette = brewer.pal(8, "Set1")
MELEE_COLOUR = palette[2]
RANGED_COLOUR = palette[1]

color = ifelse (isRanged, RANGED_COLOUR, MELEE_COLOUR)
minHealth = min(c(min(healthBefore), min(healthAfter)))
maxHealth = max(c(min(healthBefore), max(healthAfter)))
minDPS = min(c(min(dpsBefore), min(dpsAfter)))
maxDPS = max(c(min(dpsBefore), max(dpsAfter)))

plot(
    NULL,
    xlab="Health",
    ylab="Damage per second",
    main="NPC balance changes in v0.13.4",
    xlim=c(minHealth, maxHealth),
    ylim=c(minDPS, maxDPS)
)

Arrows(healthBefore,dpsBefore,healthAfter,dpsAfter,
    col=color,
    lwd=1.5,
    arr.width=0.2,
    arr.length=0.3,
    arr.type="triangle"
)

legend(
    "bottomright",
    c("Melee", "Ranged"),
    fill=c(MELEE_COLOUR, RANGED_COLOUR)
)

dev.off()