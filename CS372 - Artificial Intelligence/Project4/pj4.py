from numpy import log as ln

# Return the number of emails in the train file 
# and a dictionary that map each word to the number of emails that contain it.
def get_data(filename):
    dictionary = dict()
    count = 0
    file = open(filename)
    words = set()
    for line in file:
        line = line.rstrip()
        if line != "<SUBJECT>" and line != "</SUBJECT>" and line != "<BODY>" and line != "</BODY>" and line:
            line = line.lower()
            for word in line.split(" "):
                words.add(word)
        if line == "</BODY>":    
            count += 1
            for word in words:
                if word not in dictionary.keys():
                    dictionary[word] = 1
                else:
                    dictionary[word] += 1
            words = set()
    file.close()
    return count, dictionary


# Calculate the likelihood of a hypothesis for an email (sum of the conditional probabilities,the prior probability is not included) 
#       emails_count: total number of emails
#       features: a dictionary that map each word to the number of emails that contain it.
#       evidence: a set of words appeared in the email
def likelihood(emails_count, features, evidence):
    features_presented = 0          # number of features presented
    p = 0                           # likelihood
    for word in features.keys():
        if word in evidence:
            features_presented += 1
            p += ln((features[word] + 1) / (emails_count + 2))
        else:
            p += ln((emails_count - features[word] + 1) / (emails_count + 2))
    return features_presented, p


# Print out the predictions
#       filename: name of the test file
#       spam_count: number of spam emails in the train file
#       spam_features: a dictionary that map each word to the number of spam emails that contain it.
#       ham_count: number of ham emails in the train file
#       ham_features: a dictionary that map each word to the number of ham emails that contain it.
def predict(filename, spam_count, spam_features, ham_count, ham_features):
    # Prior probability
    spam_prob = ln(spam_count / (spam_count + ham_count))
    ham_prob = ln(ham_count / (spam_count + ham_count))
    
    hypotheses = ("spam", "ham")
    correct_answer = ""
    for hypothesis in hypotheses:
        if hypothesis in filename:
            correct_answer = hypothesis
            break  
    test = open(filename)
    evidence = set()
    emails_count = 0            # number of emails
    correct_count = 0           # number of correct predictions
    for line in test:
        line = line.rstrip()
        if line != "<SUBJECT>" and line != "</SUBJECT>" and line != "<BODY>" and line != "</BODY>" and line:
            line = line.lower()
            for word in line.split(" "):
                evidence.add(word)
        if line == "</BODY>":
            emails_count += 1
            features_count, arg_spam = likelihood(spam_count, spam_features, evidence)
            arg_spam += spam_prob       # add prior probability
            _, arg_ham = likelihood(ham_count, ham_features, evidence)
            arg_ham += ham_prob         # add prior probability
            MAP_hypothesis = "spam" if arg_spam > arg_ham else "ham"     # argmax
            if MAP_hypothesis == correct_answer:
                correctness = "right" 
                correct_count += 1
            else:
                correctness = "wrong"
            print("TEST %d %d/%d features true %.3f %.3f %s %s" % (emails_count, features_count, len(spam_features), arg_spam, arg_ham, MAP_hypothesis, correctness))
            evidence = set()
    test.close()
    
    return emails_count, correct_count

       
def main():
    # Get data from train files
    spam_count, spam_features = get_data("train-spam.txt")
    ham_count, ham_features = get_data("train-ham.txt")
    
    # A set of all features from both files
    all_features = set()
    for feature in spam_features:
        all_features.add(feature)
    for feature in ham_features:
        all_features.add(feature)

    # Add features that were not incuded to each dictionary
    for feature in all_features:
        if feature not in spam_features:
            spam_features[feature] = 0
        if feature not in ham_features:
            ham_features[feature] = 0

    test_spam_emails, correct_predictions1 = predict("test-spam.txt", spam_count, spam_features, ham_count, ham_features)
    test_ham_emails, correct_predictions2 = predict("test-ham.txt", spam_count, spam_features, ham_count, ham_features)
    
    print("Total: %d/%d emails classified correctly." % (correct_predictions1 + correct_predictions2, test_spam_emails + test_ham_emails))
    
main()      