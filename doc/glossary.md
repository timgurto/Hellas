# Glossary of ambiguous terms
[Home](index.md)

**action**, a timed endeavor, of which any user can be engaged in up to one.  The actions are:
* Gathering items from an appropriate object.  When the time elapses, the object will yield a random item in a random quantity, and then disappear if empty.
* Crafting an item, following a recipe.  When the time elapses, the user loses the materials and gains the new product.
* Constructing an object from an item.  When the time elapses, the object appears in the world and the user loses the item.
* Deconstructing an object into an item.  When the time elapses, the object disappears and the user receives the item.
* Attacking an NPC.  When the time elapses, the target loses health once it is in range, then the timer resets.  This continues while the NPC is alive and the user is 'attacking' it.

**character**, the *avatar* which represents a user in-game

**client**, the *program* which connects to the server

**construct**, when the user transforms an appropriate item from his inventory into an object

**deconstruct**, when the user transforms an appropriate object into an item in his inventory

**item**, something which exists virtually in-game, located in players' or objects' inventories (cf. *object*)

**element**, (client only) a part of the GUI, possibly containing child elements

**entity**, (client only) something which exists physically in-game, with a location and image, and supporting user interaction

**gather**, when the user performs an action on an object in order to extract items from it.  Some objects can be gathered multiple times before they disappear.

**gear**, an item that can be worn by the character in a special slot, and that may provide bonuses to the character's stats

**gear slot**, one of a set of slots in a special container that each user has access to, in addition to his inventory.  Each gear slot is numbered, and only items specifying that slot number may be equipped in that slot.

**material**, an item required by a recipe.  When the recipe is used to craft an item, the materials disappear.

**merchant object**, an object that allows exchanging one item type for another.  Merchant objects also necessarily have inventory space, with which to collect payment.

**merchant slot**, one of many potential transaction slots configured in a merchant object

**object**, something which has a physical manifestation in-game, with a location and functionality (cf. *item*)

**owner**, the user who owns an object.  Only an object's owner can access its inventory.  An object with no owner specified is accessible to all users.

**price**, the item(s) that can be given to a merchant object in one transaction, consisting of an item type and a quantity.  For example, if a merchant slot is selling 1 wood for 5 gold coins, then its "price" is 5 gold coins.

**product**, the item created by a recipe

**server**, the *program* which manages the state of the game world and connects to clients

**swap**, when a user exchanges the item in one container slot with another.  The container might be an object, or his inventory, or the gear he is wearing (cf. *take*)

**tag**, a category to which an item or object belongs, implying functionality.  For example, cutting down a tree might require a user either to have an "axe"-tagged item in his inventory, or to be near an "axe"-tagged object.  An item or object can have one or many tag, or none at all.

**take**, when a user attempts to take an item from a container slot (an object or his gear) into his inventory, without swapping another into its place (cf. *swap*)

**terminal**, a generic term for server or client

**tool**, a class of item or object that may be required to craft a recipe.  If an item it must be in the user's inventory; if an object, it must be nearby.

**user**, the *account* playing on a client.  Identified by a unique username.

**ware**, the item(s) that can be purchased from a merchant object in one transaction, consisting of an item type and a quantity.  For example, if a merchant slot is selling 1 wood for 5 gold coins, then its "ware" is 1 wood.
