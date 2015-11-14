# Yield interface
[Home](../index.md)

Yields are used to simulate realistic quantities of resources that one can gather from an object, and handles each individual gather action by players.

Each `ObjectType` that is gatherable contains a `Yield` object.  This object stores a set of normal distributions, one for each item that can be gathered, and acts as a template that is instantiated whenever a new object of a gatherable type is created.  For each item that can be gathered from an `ObjectType`, its `Yield` stores a `YieldEntry` object, which stores distribution values.

A `Yield` can be populated using the following function:
```cpp
void ObjectType::addYield(const Item *item,
                          double initMean = 1,
                          double initSD = 0,
                          double gatherMean = 1,
                          double gatherSD = 0); 
```

An example follows:
```cpp
extern Item wood, leaves, treeHeart, squirrel;
ObjectType tree("tree");

// Each tree will get ~10 wood, with SD=2.  One wood is gathered at a time.
tree.addYield(&wood, 10, 2);

// Each tree will get ~50 leaves, with SD=10.  Multiple leaves (~10, SD=5) are gathered at a time.
tree.addYield(&leaves, 50, 10, 10, 5);

// Each tree has one magic tree heart.
tree.addYield(&treeHeart);

// Trees rarely contain a squirrel (initial quantities have a floor of 0).
tree.addYield(&squirrel, -2, 1);
```

When an object is instantiated, its constructor calls `Yield::instantiate()`, which chooses random initial values based on the `_initMean` and `_initSD` of each item's `YieldEntry`, and gives the new object those quantities of items.

For example, a new 'tree' object might be given the following initial quantities of items, based on each `YieldEntry`'s mean and standard deviation:
Item      | &mu; | &sigma; | Sample initial quantities
--------- | ----:| -------:|-------------------------:
wood      | 10   | 2       | 11
leaves    | 50   | 10      | 43
treeHeart | 1    | 0       | 1
squirrel  | -2   | 1       | 0

When an object is gathered from, there are two steps involved in determining what will be gathered:

1. The object's `chooseGatherType()` function is called, which randomly chooses an item type, weighted by the quantities of each remaining item type.
2. Once the item is chosen, a quantity is chosen by calling `generateGatherQuantity()` on the object's type's `Yield`, which uses the item's `YieldEntry`'s `_gatherMean` and `_gatherSD`.

For example, using the above initial quantities, 

&nbsp;            | Remaining items                                           | Item type chosen | Quantity chosen
----------------- | --------------------------------------------------------- | -----------------|---------------:
initial           | `wwwwwwwwwwwlllllllllllllllllllllllllllllllllllllllllllt` | leaves           | 7
after 1st gather  | `wwwwwwwwwwwllllllllllllllllllllllllllllllllllllt`        | leaves           | 15
after 2nd gather  | `wwwwwwwwwwwlllllllllllllllllllllt`                       | wood             | 1
after 3rd gather  | `wwwwwwwwwwlllllllllllllllllllllt`                        | leaves           | 6
after 4th gather  | `wwwwwwwwwwlllllllllllllllt`                              | wood             | 1
after 5th gather  | `wwwwwwwwwlllllllllllllllt`                               | leaves           | 10
after 6th gather  | `wwwwwwwwwlllllt`                                         | wood             | 1
after 7th gather  | `wwwwwwwwlllllt`                                          | wood             | 1
after 8th gather  | `wwwwwwwlllllt`                                           | treeHeart        | 1
after 9th gather  | `wwwwwwwlllll`                                            | leaves           | 13
after 10th gather | `wwwwwww`                                                 | wood             | 1
after 11th gather | `wwwwww`                                                  | wood             | 1
after 12th gather | `wwwww`                                                   | wood             | 1
after 13th gather | `wwww`                                                    | wood             | 1
after 14th gather | `www`                                                     | wood             | 1
after 15th gather | `ww`                                                      | wood             | 1
after 16th gather | `w`                                                       | wood             | 1
after 17th gather |                                                           |                  |

In this example, the following items are received on successive gathers:

1. 7 leaves
2. 15 leaves
3. 1 wood
4. 6 leaves
5. 1 wood
6. 10 leaves
7. 1 wood
8. 1 wood
9. 1 treeHeart
10. 13 leaves
11. 1 wood
12. 1 wood
13. 1 wood
14. 1 wood
15. 1 wood
16. 1 wood
17. 1 wood

Note that there is a proposal for a new method of choosing the item type to gather, detailed in issue [#149](https://github.com/timgurto/mmo/issues/149).
