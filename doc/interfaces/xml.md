# XML Reading and Writing
[Home](../index.md)

Two wrapper classes exist for the TinyXML library: `XmlReader` and `XmlWriter`.  An object of either class operates on a single file at a time.

Neither class understands text inside elements.  `XmlWriter` cannot write it, and `XmlReader` will ignore it, *e.g.*:
```xml
<root>
	<h1>This text will be ignored</h1>
</root>
```

## Reading from XML
### Getting started
When an `XmlReader` object is created, a filename must be specified (either as a `const char *`, or an `std::string`), *e.g.*,
```cpp
XmlReader xr("data.xml");
```
One `XmlReader` can be reused for multiple files, in the following way:
```cpp
xr.newFile("data2.xml");
```

Once the object is created, its validity can be tested with the `!` operator or by an implicit cast to `bool`.  The object is considered valid if the file exists, and contains a root XML element.  The `newFile()` function also returns this `bool` value.
```cpp
XmlReader xr("data.xml");
assert(xr);
assert(xr.newFile("data2.xml");
```

### Navigating the tree
Navigation of the file requires the use of `TiXmlElement *` pointers to specify elements, but the use of `auto` with the class will make the code look somewhat neater.

The code samples in this section will be reading from the following small XML file:
```xml
<root>
	<particleProfile
		id="tree"
		particlesPerSecond=7 >
		<particlesPerHit mean=5 sd=1 />
		<distance mean=5 sd=0 />
		<altitude mean=20 sd=4 />
		<velocity mean=15 sd=3 />
		<fallSpeed mean=0 sd=3 />

		<variety imageFile="wood"   x=-2 y=-2 w=4 h=4 />
		<variety imageFile="leaves" x=-2 y=-2 w=4 h=4 />
	</particleProfile>
	<particleProfile id="default" />
</root>
```

#### Find a child element
The `findChild()` function returns the first child of a specified element, that has a certain value.  If a parent element is not specified, then the function looks for children of the root node:
```cpp
auto profile = findChild("particleProfile");
auto distance = findChild("distance", profile);
```
If there is no matching child, `nullptr` is returned.

#### Find multiple children of one type
Unlike `findChild()`, the `getChildren()` function returns a set of all matching children, or an empty set if there are none:
```cpp
for (auto profile : xr.getChildren("particleProfile")){
	auto varietiesSet = xr.getChildren("variety", profile);
}
```

#### Find an attribute of an element
Once you have the element you want, you will likely want to access its attributes.  To find an attribute of a specified element, use the `findAttr()` function, providing the element, the name of the attribute, and a variable to store the result:
```cpp
auto profile = xr.findChild("particleProfile");
std::string id;
bool idExists = xr.findAttr(profile, "id", id);
if (idExists)
	std::cout << "ID: " << id << std::endl;
}
```
The provided storage variable has its value changed if and only if the attribute is found.  Any value type can be used as long as it has an input operator from `std::istringstream` defined, *i.e.*, any type that can be read from `std::cin`.

The function returns `true` if the attribute was found and `false` otherwise.

This interface may appear quite verbose or unintuitive, but it allows for prettier calling code that is very readable.  For example:
```cpp
int n;
auto variety = xr.findChild("variety", profile);
if (xr.findAttr(variety, "x", n) std::cout << "x: " << n << std::endl;
if (xr.findAttr(variety, "y", n) std::cout << "y: " << n << std::endl;
if (xr.findAttr(variety, "w", n) std::cout << "w: " << n << std::endl;
if (xr.findAttr(variety, "h", n) std::cout << "h: " << n << std::endl;
```


## Writing to XML
### Getting started
When an `XmlWriter` object is created, a filename must be specified (either as a `const char *`, or an `std::string`), *e.g.*,
```cpp
XmlWriter xw("data.xml");
```
One `XmlReader` can be reused for multiple files, in the following way:
```cpp
xw.newFile("data2.xml");
```

Once the object is created, its validity can be tested with the `!` operator or by an implicit cast to `bool`.  The object is considered valid if the file exists, and contains a root XML element.
```cpp
XmlReader xr("data.xml");
assert(xr);
```

### Constructing the tree
Similarly to reading, writing XML with this interface also utilizes `TiXmlElement *` pointers.


#### Adding a child element
A new element can be added very easily, using the `addChild()` function and specifying its parent element.  If no parent is specified, then the element is added as a child of the root element:
```cpp
auto profile = xw.addChild("particleProfile");
auto velocity = xw.addChild("velocity", profile);
```

#### Adding an attribute
Attributes are also very easy to add, using the `setAttr()` function.  The element, attribute name, and attribute value must be specified.  Any value type can be used as long as it has an output operator to `std::ostringstream` defined, *i.e.*, any type that canm be written to `std::cout`:
```cpp
auto profile = xw.addChild("particleProfile");
xw.setAttr(profile, "id", "tree");
xw.setAttr(profile, "particlesPerSecond", 7);
```

### Writing the file
Once the XML tree has been completed, writing it to the file is as simple as calling `publish()`:
```cpp
xw.publish();
```
