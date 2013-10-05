//  Powiter
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */

#ifndef KNOB_H
#define KNOB_H

#include <vector>
#include <string>
#include <map>
#include <cfloat>

#include <QtGui/QVector4D>
#include <QtCore/QVariant>
#include <QtCore/QStringList>

#include "Global/GlobalDefines.h"
#include "Global/AppManager.h"

class Knob;
class Node;

/******************************VARIANT**************************************/

/*Variant type used by knob storage type.
 
 <<Do not use explicit in front of the constructors to
 allow type conversion!. This API should be
 transparant for the plug-in programmer.>>
 
 NO!
 
 from C++ Coding Standards: 101 Rules, Guidelines, and Best Practices:
 40. Avoid providing implicit conversions.
 Not all change is progress: Implicit conversions can often do more damage than good.
 Think twice before providing implicit conversions to and from the types you define,
 and prefer to rely on explicit conversions (explicit constructors and named conversion functions).
 
 setValue() is an explicit conversion function and works great.
 Don't pessimize prematurely (one line of code should run a cascade of implicit conversions)
 The code is not more difficult to read, and implicit conversions are avoided.
 Read the book above for more details on why implicit conversions are evil.
 */
//
// Inheritance is private here to avoid exposing implicitely all methods of QVariant,
// but public inheritance should work as well.
// Is there a reason for non-public inheritance?
// Note [FD]: in the previous version, the QVariant was stored as a pointer, and was
// never deleted.
class Variant : private QVariant {
public:
    
    Variant()
    : QVariant()
    {
    }
    
    explicit Variant(const QVariant& qtVariant)
    : QVariant(qtVariant)
    {
    }
    
    template<typename T>
    Variant(T variant[] ,int count)
    {
        QList<QVariant> list;
        for (int i = 0; i < count; ++i) {
            list << QVariant(variant[i]);
        }
        QVariant::setValue(list);
    }
    
    virtual ~Variant()
    {
    }
    
    template<typename T>
    T value() const {
        return QVariant::value<T>();
    }
    
    template<typename T>
    void setValue(const T &value) {
        QVariant::setValue(value);
    }
    
    template<typename T>
    void setValue(T variant[],int count){
        QList<QVariant> list;
        for (int i = 0; i < count; ++i) {
            list << QVariant(variant[i]);
        }
        QVariant::setValue(list);
    }
    
    
    int toInt(bool *ok = 0) const { return QVariant::toInt(ok); }
    uint toUInt(bool *ok = 0) const { return QVariant::toUInt(ok); }
    qlonglong toLongLong(bool *ok = 0) const { return QVariant::toLongLong(ok); }
    qulonglong toULongLong(bool *ok = 0) const { return QVariant::toULongLong(ok); }
    bool toBool() const { return QVariant::toBool(); }
    double toDouble(bool *ok = 0) const { return QVariant::toDouble(ok); }
    float toFloat(bool *ok = 0) const { return QVariant::toFloat(ok); }
    qreal toReal(bool *ok = 0) const { return QVariant::toReal(ok); }
    QByteArray toByteArray() const { return QVariant::toByteArray(); }
    //QBitArray toBitArray() const { return QVariant::toBitArray(); }
    QString toString() const { return QVariant::toString(); }
    QStringList toStringList() const { return QVariant::toStringList(); }
    QChar toChar() const { return QVariant::toChar(); }
    //QDate toDate() const { return QVariant::toDate(); }
    //QTime toTime() const { return QVariant::toTime(); }
    //QDateTime toDateTime() const { return QVariant::toDateTime(); }
    QList<QVariant> toList() const { return QVariant::toList(); }
    QMap<QString, QVariant> toMap() const { return QVariant::toMap(); }
    QHash<QString, QVariant> toHash() const { return QVariant::toHash(); }
    
    bool isNull() const{return QVariant::isNull();}
};
Q_DECLARE_METATYPE(Variant);

// specializations of setValue() for simple string types (to avoid conversions)
template<>
inline void Variant::setValue(const std::string& str){
    QVariant::setValue(QString(str.c_str()));
}

template<>
inline void Variant::setValue(const char* const& str){
    QVariant::setValue(QString(str));
}

namespace Powiter {
    class LibraryBinary;
}

/******************************KNOB_FACTORY**************************************/
//Maybe the factory should move to a separate file since it is used to create KnobGui aswell
class KnobGui;
class KnobFactory{
    
    std::map<std::string,Powiter::LibraryBinary*> _loadedKnobs;
    
public:
    KnobFactory();
    
    ~KnobFactory();
    
    const std::map<std::string,Powiter::LibraryBinary*>& getLoadedKnobs(){return _loadedKnobs;}
    
    Knob* createKnob(const std::string& name, Node* node, const std::string& description,int dimension = 1);
    
    KnobGui* createGuiForKnob(Knob* knob);
    
private:
    
    void loadKnobPlugins();
    
    void loadBultinKnobs();
    
};


/******************************KNOB_BASE**************************************/

class Knob : public QObject
{
    Q_OBJECT
    
public:    
    
    Knob(Node* node,const std::string& description,int dimension = 1);
    
    virtual ~Knob();
    
    const std::string& getDescription() const { return _description; }
    
    const std::vector<U64>& getHashVector() const { return _hashVector; }
    
    Node* getNode() const { return _node; }
    
    int getDimension() const {return _dimension;}
    
    /*Must return the name of the knob. This name will be used by the KnobFactory
     to create an instance of this knob.*/
    virtual const std::string name()=0;
    
    virtual std::string serialize() const =0;
    
    void restoreFromString(const std::string& str){
        _restoreFromString(str);
        fillHashVector();
        emit valueChanged(_value);
        tryStartRendering();
    }
    
    template<typename T>
    void setValue(T variant[],int count){
        _value = Variant(variant,count);
        fillHashVector();
        emit valueChanged(_value);
        tryStartRendering();
    }
    
    template<typename T>
    void setValue(const T &value) {
        _value.setValue(value);
        fillHashVector();
        emit valueChanged(_value);
        tryStartRendering();
    }
    
    /*Used to extract the value held by the knob.
     Derived classes should provide a more appropriate
     way to retrieve results in the expected type.*/
    template<typename T>
    T value(){
        return _value.value<T>();
    }
    
    const Variant& getValueAsVariant() const { return _value; }
    
    /*You can call this when you want to remove this Knob
     at anytime.*/
    void deleteKnob();
    
    void turnOffNewLine();
    
    void setSpacingBetweenItems(int spacing);
    
    void setEnabled(bool b);
    
    void setVisible(bool b);
    
    void setParentKnob(Knob* knob){_parentKnob = knob;}
    
    Knob* getParentKnob() const {return _parentKnob;}
    
    int determineHierarchySize() const;
    
    bool isVisible() const {return _visible;}
    
    bool isEnabled() const {return _enabled;}
    
    void turnOffUndoRedo() {_canUndo = false;}
    
    bool canBeUndone() const {return _canUndo;}
    
    void setHintToolTip(const std::string& hint){_tooltipHint = hint;}
    
    const std::string& getHintToolTip() const {return _tooltipHint;}
    
    public slots:
    /*Set the value of the knob but does NOT emit the valueChanged signal.
     This is called by the GUI*/
    void onValueChanged(const Variant& variant){
        _value = variant;
        fillHashVector();
        emit valueChangedByUser();
        tryStartRendering();
    }
    
signals:
    /*Emitted whenever the slot onValueChanged is called.
     This notifies listeners that the value held by the
     knob has changed by a user interaction.
     You can connect to this signal to do
     whatever processing you want.*/
    void valueChangedByUser();
    
    /*Emitted when the value is changed internally*/
    void valueChanged(const Variant&);
    
    /*emitted by deleteKnob().
     WARNING: To properly delete the gui
     associated, NodeGui::initializeKnobs must
     have been called. That means you should
     never call this function right away after
     KnobFactory::createKnob().*/
    void deleteWanted();
    
    void visible(bool);
    
    void enabled(bool);
    
protected:
    virtual void fillHashVector()=0; // function to add the specific values of the knob to the values vector.
    
    virtual void _restoreFromString(const std::string& str) =0;
    
    
    /*Should implement this to start rendering by calling startRendering.
     This is called by the onValueChanged() slot. If you don't want to
     start rendering, overload this function and make it do nothing.*/
    virtual void tryStartRendering() = 0;
    
    //Called to tell the engine to start processing
    //This is can be called in the implementation of tryStartRendering
    void startRendering(bool initViewer);
    
    Node *_node;
    Variant _value;
    std::vector<U64> _hashVector;
    int _dimension;
    
    
private:
    
    std::string _description;
    
    bool _newLine;
    int _itemSpacing;
    
    Knob* _parentKnob;
    bool _visible;
    bool _enabled;
    bool _canUndo;
    std::string _tooltipHint;
};

//std::vector<Knob::Knob_Flags> Knob_Mask_to_Knobs_Flags(const Knob_Mask& m);

/******************************FILE_KNOB**************************************/

class File_Knob:public Knob
{
    Q_OBJECT
    
    std::map<int,QString> _filesSequence;///mapping <frameNumber,fileName>
public:
    
    static Knob* BuildKnob(Node* node, const std::string& description,int dimension){
        return new File_Knob(node,description,dimension);
    }
    
    File_Knob(Node* node, const std::string& description,int dimension):
    Knob(node,description,dimension)
    {}
    
    virtual void fillHashVector();
    
    virtual const std::string name(){return "InputFile";}
    
    virtual std::string serialize() const;
    
    void openFile(){
        emit shouldOpenFile();
    }
    
    /**
     * @brief firstFrame
     * @return Returns the index of the first frame in the sequence held by this Reader.
     */
    int firstFrame() const;
    
    /**
     * @brief lastFrame
     * @return Returns the index of the last frame in the sequence held by this Reader.
     */
	int lastFrame() const;
    
    int frameCount() const{return _filesSequence.size();}
    
    /**
     * @brief nearestFrame
     * @return Returns the index of the nearest frame in the Range [ firstFrame() - lastFrame( ].
     * @param f The index of the frame to modify.
     */
    int nearestFrame(int f) const;
    
    /**
     * @brief getRandomFrameName
     * @param f The index of the frame.
     * @return The file name associated to the frame index. Returns an empty string if it couldn't find it.
     */
    const QString getRandomFrameName(int f) const;
    
signals:
    void filesSelected();
    void shouldOpenFile();
    void frameRangeChanged(int,int);
    
protected:
    
    virtual void tryStartRendering();
    
    virtual void _restoreFromString(const std::string& str);
    
private:
    
    void getVideoSequenceFromFilesList();
    
};

/******************************OUTPUT_FILE_KNOB**************************************/

class OutputFile_Knob:public Knob
{
    Q_OBJECT
public:
    
    static Knob* BuildKnob(Node* node, const std::string& description,int dimension){
        return new OutputFile_Knob(node,description,dimension);
    }
    
    OutputFile_Knob(Node* node, const std::string& description,int dimension):
    Knob(node,description,dimension)
    {}
    
    virtual void fillHashVector(){
        // filenames of the sequence should not be involved in hash key computation as it defeats all the purpose of the cache.
        // explanation: The algorithm doing the look-up in the cache already takes care of the current frame name.
        // See  VideoEngine::render(...) at line starting with  key = FrameEntry::computeHashKey to understand the call.
    }
    
    virtual const std::string name(){return "OutputFile";}
    
    virtual std::string serialize() const;
    
    void openFile(){
        emit shouldOpenFile();
    }
    
signals:
    
    void filesSelected();
    
    void shouldOpenFile();
    
protected:
    
    virtual void tryStartRendering();
    
    virtual void _restoreFromString(const std::string& str);
    
    
};

/******************************INT_KNOB**************************************/

class Int_Knob:public Knob
{
    
    Q_OBJECT
    
public:
    
    static Knob* BuildKnob(Node* node, const std::string& description,int dimension){
        return new Int_Knob(node,description,dimension);
    }
    
    Int_Knob(Node* node, const std::string& description,int dimension):
    Knob(node,description,dimension)
    {}
    
    virtual void fillHashVector();
    
    virtual const std::string name(){return "Int";}
    
    virtual std::string serialize() const;
    
    /*Returns a vector of values. The vector
     contains _dimension elements.*/
    const std::vector<int> getValues() const;
    
    void setMinimum(int mini,int index = 0){
        if(_minimums.size() > (U32)index){
            _minimums[index] = mini;
        }else{
            if(index == 0){
                _minimums.push_back(mini);
            }else{
                while(_minimums.size() <= (U32)index){
                    _minimums.push_back(INT_MIN);
                }
                _minimums.push_back(mini);
            }
        }
        int maximum = 99;
        if(_maximums.size() > (U32)index){
            maximum = _maximums[index];
        }
        emit minMaxChanged(mini,maximum,index);
    }
    
    void setMaximum(int maxi,int index = 0){
        
        if(_maximums.size() > (U32)index){
            _maximums[index] = maxi;
        }else{
            if(index == 0){
                _maximums.push_back(maxi);
            }else{
                while(_maximums.size() <= (U32)index){
                    _maximums.push_back(INT_MAX);
                }
                _maximums.push_back(maxi);
            }
        }
        int minimum = 99;
        if(_minimums.size() > (U32)index){
            minimum = _minimums[index];
        }
        emit minMaxChanged(minimum,maxi,index);
    }
    
    void setDisplayMinimum(int mini,int index = 0){
        if(_displayMins.size() > (U32)index){
            _displayMins[index] = mini;
        }else{
            if(index == 0){
                _displayMins.push_back(mini);
            }else{
                while(_displayMins.size() <= (U32)index){
                    _displayMins.push_back(0);
                }
                _displayMins.push_back(mini);
            }
        }
    }
    
    void setDisplayMaximum(int maxi,int index = 0){
        
        if(_displayMaxs.size() > (U32)index){
            _displayMaxs[index] = maxi;
        }else{
            if(index == 0){
                _displayMaxs.push_back(maxi);
            }else{
                while(_displayMaxs.size() <= (U32)index){
                    _displayMaxs.push_back(99);
                }
                _displayMaxs.push_back(maxi);
            }
        }
    }
    
    void setIncrement(int incr, int index = 0) {
        assert(incr > 0);
        /*If _increments is already filled, just replace the existing value*/
        if (_increments.size() > (U32)index) {
            _increments[index] = incr;
        }else{
            /*If it is not filled and it is 0, just push_back the value*/
            if(index == 0){
                _increments.push_back(incr);
            } else {
                /*Otherwise fill enough values until we  have
                 the corresponding index in _increments. Then we
                 can push_back the value as the last element of the
                 vector.*/
                while (_increments.size() <= (U32)index) {
                    // FIXME: explain what happens here (comment?)
                    _increments.push_back(1);
                    assert(_increments[_increments.size()-1] > 0);
                }
                _increments.push_back(incr);
            }
        }
        emit incrementChanged(_increments[index],index);
    }
    
    void setIncrement(const std::vector<int>& incr){
        _increments = incr;
        for (U32 i = 0; i < incr.size(); ++i) {
            assert(_increments[i] > 0);
            emit incrementChanged(_increments[i],i);
        }
    }
    
    /*minis & maxis must have the same size*/
    void setMinimumsAndMaximums(const std::vector<int>& minis,const std::vector<int>& maxis){
        _minimums = minis;
        _maximums = maxis;
        for (U32 i = 0; i < maxis.size(); ++i) {
            emit minMaxChanged(_minimums[i], _maximums[i],i);
        }
    }
    
    void setDisplayMinimumsAndMaximums(const std::vector<int>& minis,const std::vector<int>& maxis){
        _displayMins = minis;
        _displayMaxs = maxis;
    }
    
    const std::vector<int>& getMinimums() const {return _minimums;}
    
    const std::vector<int>& getMaximums() const {return _maximums;}
    
    const std::vector<int>& getIncrements() const {return _increments;}
    
    const std::vector<int>& getDisplayMinimums() const {return _displayMins;}
    
    const std::vector<int>& getDisplayMaximums() const {return _displayMaxs;}
    
    
protected:
    
    virtual void tryStartRendering();
    
    virtual void _restoreFromString(const std::string& str);
    
signals:
    
    void minMaxChanged(int mini,int maxi,int index = 0);
    
    void incrementChanged(int incr,int index = 0);
    
private:

    std::vector<int> _minimums,_maximums,_increments,_displayMins,_displayMaxs;
    
};

/******************************BOOL_KNOB**************************************/

class Bool_Knob:public Knob
{
    
public:
    
    static Knob* BuildKnob(Node* node, const std::string& description,int dimension){
        return new Bool_Knob(node,description,dimension);
    }
    
    Bool_Knob(Node* node, const std::string& description,int dimension):
    Knob(node,description,dimension)
    {}
    
    virtual void fillHashVector();
    
    virtual const std::string name(){return "Bool";}
    
    virtual std::string serialize() const;
    
    
    bool getValue() const { return _value.toBool(); }
    
protected:
    
    virtual void tryStartRendering();
    
    virtual void _restoreFromString(const std::string& str);
    
    
    
};

/******************************DOUBLE_KNOB**************************************/

class Double_Knob:public Knob
{
    Q_OBJECT
    
public:
    
    static Knob* BuildKnob(Node* node, const std::string& description,int dimension){
        return new Double_Knob(node,description,dimension);
    }
    
    Double_Knob(Node* node, const std::string& description,int dimension):
    Knob(node,description,dimension)
    {}
    
    virtual void fillHashVector();
    
    virtual const std::string name(){return "Double";}
    
    virtual std::string serialize() const;
    
    /*Returns a vector of values. The vector
     contains _dimension elements.*/
    const std::vector<double> getValues() const ;
    
    const std::vector<double>& getMinimums() const {return _minimums;}
    
    const std::vector<double>& getMaximums() const {return _maximums;}
    
    const std::vector<double>& getIncrements() const {return _increments;}
    
    const std::vector<int>& getDecimals() const {return _decimals;}
    
    const std::vector<double>& getDisplayMinimums() const {return _displayMins;}
    
    const std::vector<double>& getDisplayMaximums() const {return _displayMaxs;}
    
    void setMinimum(double mini,int index = 0){
        if(_minimums.size() > (U32)index){
            _minimums[index] = mini;
        }else{
            if(index == 0){
                _minimums.push_back(mini);
            }else{
                while(_minimums.size() <= (U32)index){
                    _minimums.push_back(0);
                }
                _minimums.push_back(mini);
            }
        }
        double maximum = 99;
        if(_maximums.size() > (U32)index){
            maximum = _maximums[index];
        }
        emit minMaxChanged(mini,maximum,index);
    }
    
    void setMaximum(double maxi,int index = 0){
        if(_maximums.size() > (U32)index){
            _maximums[index] = maxi;
        }else{
            if(index == 0){
                _maximums.push_back(maxi);
            }else{
                while(_maximums.size() <= (U32)index){
                    _maximums.push_back(99);
                }
                _maximums.push_back(maxi);
            }
        }
        double minimum = 99;
        if(_minimums.size() > (U32)index){
            minimum = _minimums[index];
        }
        emit minMaxChanged(minimum,maxi,index);
    }
    
    void setDisplayMinimum(double mini,int index = 0){
        if(_displayMins.size() > (U32)index){
            _displayMins[index] = mini;
        }else{
            if(index == 0){
                _displayMins.push_back(DBL_MIN);
            }else{
                while(_displayMins.size() <= (U32)index){
                    _displayMins.push_back(0);
                }
                _displayMins.push_back(mini);
            }
        }
    }
    
    void setDisplayMaximum(double maxi,int index = 0){
        
        if(_displayMaxs.size() > (U32)index){
            _displayMaxs[index] = maxi;
        }else{
            if(index == 0){
                _displayMaxs.push_back(maxi);
            }else{
                while(_displayMaxs.size() <= (U32)index){
                    _displayMaxs.push_back(DBL_MAX);
                }
                _displayMaxs.push_back(maxi);
            }
        }
    }
    
    void setIncrement(double incr, int index = 0) {
        assert(incr > 0.);
        if (_increments.size() > (U32)index) {
            _increments[index] = incr;
        }else{
            if(index == 0){
                _increments.push_back(incr);
            }else{
                while (_increments.size() <= (U32)index) {
                    _increments.push_back(0.1);  // FIXME: explain with a comment what happens here? why 0.1?
                }
                _increments.push_back(incr);
            }
        }
        emit incrementChanged(_increments[index],index);
    }
    
    void setDecimals(int decis,int index = 0){
        if(_decimals.size() > (U32)index){
            _decimals[index] = decis;
        }else{
            if(index == 0){
                _decimals.push_back(decis);
            }else{
                while(_decimals.size() <= (U32)index){
                    _decimals.push_back(3);
                    _decimals.push_back(decis);
                }
            }
        }
        emit decimalsChanged(_decimals[index],index);
    }
    
    
    /*minis & maxis must have the same size*/
    void setMinimumsAndMaximums(const std::vector<double>& minis,const std::vector<double>& maxis){
        _minimums = minis;
        _maximums = maxis;
        for (U32 i = 0; i < maxis.size(); ++i) {
            emit minMaxChanged(_minimums[i], _maximums[i],i);
        }
    }
    
    void setDisplayMinimumsAndMaximums(const std::vector<double>& minis,const std::vector<double>& maxis){
        _displayMins = minis;
        _displayMaxs = maxis;
    }
    
    void setIncrement(const std::vector<double>& incr){
        _increments = incr;
        for (U32 i = 0; i < incr.size(); ++i) {
            emit incrementChanged(_increments[i],i);
        }
    }
    void setDecimals(const std::vector<int>& decis){
        _decimals = decis;
        for (U32 i = 0; i < decis.size(); ++i) {
            emit decimalsChanged(decis[i],i);
        }
    }
    
protected:
    
    virtual void tryStartRendering();
    
    virtual void _restoreFromString(const std::string& str);
    
signals:
    void minMaxChanged(double mini,double maxi,int index = 0);
    
    void incrementChanged(double incr,int index = 0);
    
    void decimalsChanged(int deci,int index = 0);
    
private:
    
    std::vector<double> _minimums,_maximums,_increments,_displayMins,_displayMaxs;
    std::vector<int> _decimals;
    
};

/******************************BUTTON_KNOB**************************************/

class Button_Knob:public Knob
{
    Q_OBJECT
    
public:
    
    static Knob* BuildKnob(Node* node, const std::string& description,int dimension){
        return new Button_Knob(node,description,dimension);
    }
    
    Button_Knob(Node* node, const std::string& description,int dimension);
    
    virtual void fillHashVector(){}
    
    virtual const std::string name(){return "Button";}
    
    virtual std::string serialize() const{return "";}
    
    public slots:
    void connectToSlot(const char* v);
    
    
protected:
    
    virtual void tryStartRendering(){}
    
    virtual void _restoreFromString(const std::string& str){(void)str;}
    
    
    
};

/******************************COMBOBOX_KNOB**************************************/

class ComboBox_Knob:public Knob
{
public:
    
    static Knob* BuildKnob(Node* node, const std::string& description,int dimension){
        return new ComboBox_Knob(node,description,dimension);
    }
    
    ComboBox_Knob(Node* node, const std::string& description,int dimension):
    Knob(node,description,dimension)
    {
        
    }
    
    virtual void fillHashVector();
    
    virtual const std::string name(){return "ComboBox";}
    
    virtual std::string serialize() const;
    
    /*Must be called right away after the constructor.*/
    void populate(const std::vector<std::string>& entries){
        _entries = entries;
    }
    
    const std::vector<std::string>& getEntries() const {return _entries;}
    
    int getActiveEntry() const {return _value.toInt();}
    
protected:
    
    virtual void tryStartRendering();
    
    virtual void _restoreFromString(const std::string& str);
    
    
private:
    std::vector<std::string> _entries;
};

/******************************SEPARATOR_KNOB**************************************/

class Separator_Knob:public Knob
{
    
public:
    
    static Knob* BuildKnob(Node* node, const std::string& description,int dimension){
        return new Separator_Knob(node,description,dimension);
    }
    
    Separator_Knob(Node* node, const std::string& description,int dimension):
    Knob(node,description,dimension){
        
    }
    
    virtual void fillHashVector(){}
    
    virtual const std::string name(){return "Separator";}
    
    virtual std::string serialize() const{return "";}
    
protected:
    
    virtual void tryStartRendering(){}
    
    virtual void _restoreFromString(const std::string& str){(void)str;}
    
};
/******************************RGBA_KNOB**************************************/
class RGBA_Knob:public Knob
{
public:
    
    static Knob* BuildKnob(Node* node, const std::string& description,int dimension){
        return new RGBA_Knob(node,description,dimension);
    }
    
    RGBA_Knob(Node* node, const std::string& description,int dimension):
    Knob(node,description,dimension),_alphaEnabled(true)
    {
        
    }
    
    virtual void fillHashVector();
    
    virtual const std::string name(){return "RGBA";}
    
    virtual std::string serialize() const;
    
    QVector4D getValues() const {return _value.value<QVector4D>();}
    
    void setAlphaEnabled(bool enabled) { _alphaEnabled = enabled; }
    
    bool isAlphaEnabled() const { return _alphaEnabled; }
    
protected:
    
    virtual void tryStartRendering(){
        startRendering(false);
    }
    
    virtual void _restoreFromString(const std::string& str);
    
    
private:
    
    bool _alphaEnabled;
    QStringList _entries;
};

/******************************STRING_KNOB**************************************/
class String_Knob:public Knob
{
public:
    
    static Knob* BuildKnob(Node* node, const std::string& description,int dimension){
        return new String_Knob(node,description,dimension);
    }
    
    String_Knob(Node* node, const std::string& description,int dimension):
    Knob(node,description,dimension){
        
    }
    
    virtual void fillHashVector();
    
    virtual const std::string name(){return "String";}
    
    virtual std::string serialize() const;
    
    std::string getString() const {return _value.toString().toStdString();}
    
protected:
    
    virtual void tryStartRendering(){
        startRendering(true);
    }
    
    virtual void _restoreFromString(const std::string& str);
    
    
private:
    QStringList _entries;
};
/******************************GROUP_KNOB**************************************/
class Group_Knob:public Knob
{
    Q_OBJECT
    
    std::vector<Knob*> _children;
public:
    
    static Knob* BuildKnob(Node* node, const std::string& description,int dimension){
        return new Group_Knob(node,description,dimension);
    }
    
    Group_Knob(Node* node, const std::string& description,int dimension):
    Knob(node,description,dimension)
    {
        
    }
    
    virtual void fillHashVector(){}
    
    virtual const std::string name(){return "Group";}
    
    virtual std::string serialize() const{return "";}
    
    void addKnob(Knob* k);
    
    const std::vector<Knob*>& getChildren() const {return _children;}
    
protected:
    
    virtual void tryStartRendering(){}
    
    virtual void _restoreFromString(const std::string& str){(void)str;}
    
};
/******************************TAB_KNOB**************************************/

class Tab_Knob:public Knob
{
    Q_OBJECT
public:
    
    static Knob* BuildKnob(Node* node, const std::string& description,int dimension){
        return new Tab_Knob(node,description,dimension);
    }
    
    Tab_Knob(Node* node, const std::string& description,int dimension):
    Knob(node,description,dimension){
        
    }
    
    virtual void fillHashVector(){}
    
    virtual const std::string name(){return "Tab";}
    
    virtual std::string serialize() const{return "";}
    
    void addTab(const std::string& name);
    
    void addKnob(const std::string& tabName,Knob* k);
    
    const std::map<std::string,std::vector<Knob*> >& getKnobs() const {return _knobs;}
    
protected:
    
    virtual void tryStartRendering(){}
    
    virtual void _restoreFromString(const std::string& str){(void)str;}
    
private:
    std::map<std::string,std::vector<Knob*> > _knobs;
};

#endif // KNOB_H
