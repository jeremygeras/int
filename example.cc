const int call_duration_ms = 10000;

ScopedTestAccount alice("alice");
ScopedTestAccount bob("bob");

// make an outgoing (audio only) call from Alice to Bob
SipConversationHandle aliceCall = alice.conversation->createConversation(alice.handle);
alice.conversation->addParticipant(aliceCall, bob.uri);
alice.conversation->start(aliceCall);

// spawn a thread for Bob using std::async
// **************** START OF Bob's thread ******************

std::async(std::launch::async, [&]() {
   SipConversationHandle bobCall = 0;
   {
      SipConversationHandle h;
      NewConversationEvent evt;
      ASSERT_TRUE(cpcExpectEvent(bob.events,
         "SipConversationHandler::onNewConversation", // wait for an event called onNewConversation to happen
         5000,
         AlwaysTruePred(),
         h, evt));
      bobCall = h;
      ASSERT_EQ(evt.account, bob.handle);
      ASSERT_EQ(evt.conversationState, ConversationState_RemoteOriginated);
      ASSERT_EQ(evt.conversationType, ConversationType_Incoming);
      ASSERT_EQ(evt.remoteAddress, alice.uri);
      ASSERT_EQ(evt.remoteDisplayName, alice.name);
      ASSERT_EQ(evt.remoteMediaInfo.size(), 1);
   }

   ASSERT_EQ(bob.conversation->sendRingingResponse(bobCall), kSuccess);

   {
      SipConversationHandle h;
      ConversationStateChangedEvent evt;
      ASSERT_TRUE(cpcExpectEvent(bob.events,
         "SipConversationHandler::onConversationStateChanged",
         5000,
         HandleEqualsPred<SipConversationHandle>(bobCall),
         h, evt));
      ASSERT_EQ(evt.conversationState, ConversationState_LocalRinging);
   }

   ASSERT_EQ(bob.conversation->accept(bobCall), kSuccess);

   {
      SipConversationHandle h;
      ConversationMediaChangedEvent evt;
      ASSERT_TRUE(cpcExpectEvent(bob.events,
         "SipConversationHandler::onConversationMediaChanged",
         15000,
         HandleEqualsPred<SipConversationHandle>(bobCall),
         h, evt));
      ASSERT_EQ(evt.localMediaInfo.size(), 1);
      ASSERT_EQ(evt.localMediaInfo.at(0).mediaType, MediaType_Audio);
      ASSERT_EQ(evt.localMediaInfo.at(0).mediaDirection, MediaDirection_SendReceive);
   }

         {
            SipConversationHandle h;
            ConversationStateChangedEvent evt;
            ASSERT_TRUE(cpcExpectEvent(bob.events,
               "SipConversationHandler::onConversationStateChanged",
               5000,
               HandleEqualsPred<SipConversationHandle>(bobCall),
               h, evt));
            ASSERT_EQ(evt.conversationState, ConversationState_Connected);
         }

         std::this_thread::sleep_for(std::chrono::milliseconds(call_duration_ms));

         ASSERT_EQ(bob.conversation->end(bobCall), kSuccess);

         {
            SipConversationHandle h;
            ConversationEndedEvent evt;
            ASSERT_TRUE(cpcExpectEvent(bob.events,
               "SipConversationHandler::onConversationEnded",
               5000,
               HandleEqualsPred<SipConversationHandle>(bobCall),
               h, evt));
            ASSERT_EQ(evt.endReason, ConversationEndReason_UserTerminatedLocally);
         }
});

// **************** END OF Bob's thread ******************


// **************** START OF Alice's thread ******************

std::async(std::launch::async, [&]() {

   {
      SipConversationHandle h;
      NewConversationEvent evt;
      ASSERT_TRUE(cpcExpectEvent(alice.events,
         "SipConversationHandler::onNewConversation",
         15000,
         HandleEqualsPred<SipConversationHandle>(aliceCall),
         h, evt));
      ASSERT_EQ(evt.account, alice.handle);
      ASSERT_EQ(evt.conversationType, ConversationType_Outgoing);
      ASSERT_EQ(evt.remoteAddress, bob.uri);
      ASSERT_EQ(evt.remoteDisplayName, L"");
      ASSERT_EQ(evt.localMediaInfo.size(), 1);
   }

         {
            SipConversationHandle h;
            ConversationStateChangedEvent evt;
            ASSERT_TRUE(cpcExpectEvent(alice.events,
               "SipConversationHandler::onConversationStateChanged",
               5000,
               HandleEqualsPred<SipConversationHandle>(aliceCall),
               h, evt));
            ASSERT_EQ(evt.conversationState, ConversationState_RemoteRinging);
         }

         {
            SipConversationHandle h;
            ConversationMediaChangedEvent evt;
            ASSERT_TRUE(cpcExpectEvent(alice.events,
               "SipConversationHandler::onConversationMediaChanged",
               5000,
               HandleEqualsPred<SipConversationHandle>(aliceCall),
               h, evt));
            ASSERT_EQ(evt.localMediaInfo.size(), 1);
            ASSERT_EQ(evt.localMediaInfo.at(0).mediaType, MediaType_Audio);
            ASSERT_EQ(evt.localMediaInfo.at(0).mediaDirection, MediaDirection_SendReceive);
         }

         {
            SipConversationHandle h;
            ConversationStateChangedEvent evt;
            ASSERT_TRUE(cpcExpectEvent(alice.events,
               "SipConversationHandler::onConversationStateChanged",
               5000,
               HandleEqualsPred<SipConversationHandle>(aliceCall),
               h, evt));
            ASSERT_EQ(evt.conversationState, ConversationState_Connected);
         }

         {
            SipConversationHandle h;
            ConversationEndedEvent evt;
            ASSERT_TRUE(cpcExpectEvent(alice.events,
               "SipConversationHandler::onConversationEnded",
               call_duration_ms + 1000,
               HandleEqualsPred<SipConversationHandle>(aliceCall),
               h, evt));
            ASSERT_EQ(evt.endReason, ConversationEndReason_UserTerminatedRemotely);
         }
});
// **************** END OF Alice's thread ******************




