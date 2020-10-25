/*
        qdate.h

        Custom serializer for Qt's QDate type as xsd:date.
        
        Requires Qt 4.8 or higher.
        
        When serializing: If the QDate is invalid, it will be 
        defaulted to 01-01-01.
        When deserializing: If a timezone is found, it will
        be checked for validity and then ignored, as
        QDate's don't handle timezones.

        #import this file into your gSOAP .h file.

        To automate the wsdl2h-mapping of xsd:date to
        QDate, add this line to the typemap.dat file:

        xsd__date = #import "custom/qdate.h" | xsd__date 

        When using soapcpp2 option -q<name> or -p<name>, you must change
        qdate.cpp as follows:

                #include "soapH.h"  ->  #include "nameH.h"

        Compile and link your code with custom/qdate.cpp

gSOAP XML Web services tools
Copyright (C) 2000-2016, Robert van Engelen, Genivia Inc., All Rights Reserved.
This part of the software is released under ONE of the following licenses:
 Genivia's license for commercial use.
--------------------------------------------------------------------------------
gSOAP public license.

The contents of this file are subject to the gSOAP Public License Version 1.3
(the "License"); you may not use this file except in compliance with the
License. You may obtain a copy of the License at
http://www.cs.fsu.edu/~engelen/soaplicense.html
Software distributed under the License is distributed on an "AS IS" basis,
WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
for the specific language governing rights and limitations under the License.

The Initial Developer of the Original Code is Robert A. van Engelen.
Copyright (C) 2000-2016, Robert van Engelen, Genivia, Inc., All Rights Reserved.
--------------------------------------------------------------------------------
Product and source code licensed by Genivia, Inc., contact@genivia.com
--------------------------------------------------------------------------------
*/

#include <QDate>

extern class QDate;

extern typedef QDate xsd__date;
