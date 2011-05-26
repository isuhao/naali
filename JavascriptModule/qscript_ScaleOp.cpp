#include "QtScriptBindingsHelpers.h"

static QScriptValue ScaleOp_ScaleOp(QScriptContext *context, QScriptEngine *engine)
{
    ScaleOp ret;
    return TypeToQScriptValue(engine, ret);
}

static QScriptValue ScaleOp_ScaleOp_float3(QScriptContext *context, QScriptEngine *engine)
{
    float3 scale = TypeFromQScriptValue<float3>(context->argument(0));
    ScaleOp ret(scale);
    return TypeToQScriptValue(engine, ret);
}

static QScriptValue ScaleOp_ScaleOp_float_float_float(QScriptContext *context, QScriptEngine *engine)
{
    float sx = TypeFromQScriptValue<float>(context->argument(0));
    float sy = TypeFromQScriptValue<float>(context->argument(1));
    float sz = TypeFromQScriptValue<float>(context->argument(2));
    ScaleOp ret(sx, sy, sz);
    return TypeToQScriptValue(engine, ret);
}

static QScriptValue ScaleOp_Offset(QScriptContext *context, QScriptEngine *engine)
{
    ScaleOp *This = TypeFromQScriptValue<ScaleOp*>(context->thisObject());
    if (!This) { printf("Error! Invalid context->thisObject in file %s, line %d\n!", __FILE__, __LINE__); return QScriptValue(); }
    float3 ret = This->Offset();
    return TypeToQScriptValue(engine, ret);
}

static QScriptValue ScaleOp_ctor(QScriptContext *context, QScriptEngine *engine)
{
    if (context->argumentCount() == 0)
        return ScaleOp_ScaleOp(context, engine);
    if (context->argumentCount() == 1 && QSVIsOfType<float3>(context->argument(0)))
        return ScaleOp_ScaleOp_float3(context, engine);
    if (context->argumentCount() == 3 && QSVIsOfType<float>(context->argument(0)) && QSVIsOfType<float>(context->argument(1)) && QSVIsOfType<float>(context->argument(2)))
        return ScaleOp_ScaleOp_float_float_float(context, engine);
    printf("ScaleOp_ctor failed to choose the right function to call! Did you use 'var x = ScaleOp();' instead of 'var x = new ScaleOp();'?\n"); return QScriptValue();
}

class ScaleOp_scriptclass : public QScriptClass
{
public:
    QScriptValue objectPrototype;
    ScaleOp_scriptclass(QScriptEngine *engine):QScriptClass(engine){}
    QScriptValue property(const QScriptValue &object, const QScriptString &name, uint id)
    {
        ScaleOp *This = TypeFromQScriptValue<ScaleOp*>(object);
        if ((QString)name == (QString)"x") return TypeToQScriptValue(engine(), This->x);
        if ((QString)name == (QString)"y") return TypeToQScriptValue(engine(), This->y);
        if ((QString)name == (QString)"z") return TypeToQScriptValue(engine(), This->z);
        return QScriptValue();
    }
    void setProperty(QScriptValue &object, const QScriptString &name, uint id, const QScriptValue &value)
    {
        ScaleOp *This = TypeFromQScriptValue<ScaleOp*>(object);
        if ((QString)name == (QString)"x") This->x = TypeFromQScriptValue<float>(value);
        if ((QString)name == (QString)"y") This->y = TypeFromQScriptValue<float>(value);
        if ((QString)name == (QString)"z") This->z = TypeFromQScriptValue<float>(value);
    }
    QueryFlags queryProperty(const QScriptValue &object, const QScriptString &name, QueryFlags flags, uint *id)
    {
        if ((QString)name == (QString)"x") return flags;
        if ((QString)name == (QString)"y") return flags;
        if ((QString)name == (QString)"z") return flags;
        return 0;
    }
    QScriptValue prototype() const { return objectPrototype; }
};
QScriptValue register_ScaleOp_prototype(QScriptEngine *engine)
{
    engine->setDefaultPrototype(qMetaTypeId<ScaleOp*>(), QScriptValue());
    QScriptValue proto = engine->newVariant(qVariantFromValue((ScaleOp*)0));
    proto.setProperty("Offset", engine->newFunction(ScaleOp_Offset, 0));
    ScaleOp_scriptclass *sc = new ScaleOp_scriptclass(engine);
    engine->setProperty("ScaleOp_scriptclass", QVariant::fromValue<QScriptClass*>(sc));
    proto.setScriptClass(sc);
    sc->objectPrototype = proto;
    engine->setDefaultPrototype(qMetaTypeId<ScaleOp>(), proto);
    engine->setDefaultPrototype(qMetaTypeId<ScaleOp*>(), proto);
    QScriptValue ctor = engine->newFunction(ScaleOp_ctor, proto, 3);
    engine->globalObject().setProperty("ScaleOp", ctor);
    return ctor;
}
