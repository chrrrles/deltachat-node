/* eslint-disable camelcase */
const binding = require('./binding')
const EventEmitter = require('events').EventEmitter
const inherits = require('util').inherits
const xtend = require('xtend')
const path = require('path')
const debug = require('debug')('deltachat')
const camelCase = require('camelcase')

const DEFAULTS = { root: process.cwd() }

function DeltaChat (opts) {
  if (!(this instanceof DeltaChat)) {
    return new DeltaChat(opts)
  }

  EventEmitter.call(this)

  opts = xtend(DEFAULTS, opts || {})

  if (typeof opts.email !== 'string') throw new Error('Missing .email')
  if (typeof opts.password !== 'string') throw new Error('Missing .password')

  const dcn_context = binding.dcn_context_new()
  this.binding = binding.dcn_context_t

  Object.keys(this.binding).forEach(key => {
    const camel = camelCase(key.replace('dc_', ''))
    if (typeof this[camel] === 'function') {
      debug('method', camel, 'already exists, skipping!')
    } else {
      debug('binding', camel, 'to', key)
      this[camel] = (...args) => {
        args = [ dcn_context ].concat(args)
        return this.binding[key].apply(null, args)
      }
    }
  })

  this.setEventHandler((event, data1, data2) => {
    debug('event', event, 'data1', data1, 'data2', data2)
  })

  this.open(path.join(opts.root, 'db.sqlite'), '')

  if (!this.isConfigured()) {
    this.setConfig('addr', opts.email)
    this.setConfig('mail_pw', opts.password)
    this.configure()
  }

  this.startThreads()
}

DeltaChat.prototype.checkQr = function (qr) {
  throw new Error('checkQr NYI')
}

DeltaChat.prototype.getBlockedContacts = function () {
  throw new Error('getBlockedContacts NYI')
}

DeltaChat.prototype.getChat = function (chatId) {
  throw new Error('getChat NYI')
}

DeltaChat.prototype.getChatContacts = function (chatId) {
  throw new Error('getChatContacts NYI')
}

DeltaChat.prototype.getChatMedia = function (chatId, msgType, orMsgType) {
  throw new Error('getChatMedia NYI')
}

DeltaChat.prototype.getChatMsgs = function (chatId, flags, marker1Before) {
  throw new Error('getChatMsgs NYI')
}

DeltaChat.prototype.getChatList = function (listFlags, queryStr, queryId) {
  throw new Error('getChatList NYI')
}

DeltaChat.prototype.getContact = function (contactId) {
  throw new Error('getContact NYI')
}

DeltaChat.prototype.getContact = function (contactId) {
  throw new Error('getContact NYI')
}

DeltaChat.prototype.getContacts = function (listFlags, query) {
  throw new Error('getContacts NYI')
}

DeltaChat.prototype.getFreshMsgs = function () {
  throw new Error('getFreshMsgs NYI')
}

DeltaChat.prototype.getMsg = function (msgId) {
  throw new Error('getMsg NYI')
}

DeltaChat.prototype.searchMsgs = function (chatId, query) {
  throw new Error('searchMsgs NYI')
}

inherits(DeltaChat, EventEmitter)

module.exports = DeltaChat
